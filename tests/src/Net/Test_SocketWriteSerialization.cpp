#include <catch.hpp>

#include <Net/Socket.h>
#include <Net/SocketAddress.h>

#include <asio.hpp>

#include <chrono>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

namespace
{
	// Frame on the wire: uint32 little-endian payload length, uint32 little-endian
	// id, then payload bytes. Socket::SendAsync writes raw buffers, so the
	// receiver parses this self-describing stream to prove that queued writes
	// arrive intact, in order, with no interleaving or buffer reuse corruption.
	std::vector<uint8_t> MakeFrame(const uint32_t id, const size_t payloadSize, const uint8_t fill)
	{
		std::vector<uint8_t> frame(8 + payloadSize);
		for (int byte = 0; byte < 4; ++byte) {
			frame[byte] = (uint8_t)((payloadSize >> (8 * byte)) & 0xFF);
			frame[4 + byte] = (uint8_t)((id >> (8 * byte)) & 0xFF);
		}
		std::memset(frame.data() + 8, fill, payloadSize);
		return frame;
	}

	uint32_t ReadU32(const std::vector<uint8_t>& bytes, const size_t offset)
	{
		uint32_t value = 0;
		for (int byte = 0; byte < 4; ++byte) {
			value |= (uint32_t)bytes[offset + byte] << (8 * byte);
		}
		return value;
	}

	std::vector<uint8_t> ReadUntil(
		asio::ip::tcp::socket& socket,
		const size_t expectedSize,
		const std::chrono::seconds timeout)
	{
		std::vector<uint8_t> received;
		received.reserve(expectedSize);
		std::vector<uint8_t> chunk(16 * 1024);
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (received.size() < expectedSize && std::chrono::steady_clock::now() < deadline) {
			asio::error_code ec;
			const size_t numRead = socket.read_some(asio::buffer(chunk), ec);
			if (!ec) {
				received.insert(received.end(), chunk.begin(), chunk.begin() + numRead);
			} else if (ec == asio::error::would_block || ec == asio::error::try_again) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			} else {
				break;
			}
		}
		return received;
	}

	struct TestSocket
	{
		std::shared_ptr<asio::io_context> pClientContext;
		std::shared_ptr<asio::io_context> pServerContext;
		std::shared_ptr<asio::executor_work_guard<asio::io_context::executor_type>> pClientWorkGuard;
		std::shared_ptr<asio::ip::tcp::socket> pClientSocket;
		std::shared_ptr<asio::ip::tcp::socket> pServerSocket;
		std::shared_ptr<Socket> pSocket;
		std::thread ioThread;

		TestSocket()
		{
			pServerContext = std::make_shared<asio::io_context>();
			asio::ip::tcp::acceptor acceptor(
				*pServerContext,
				asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), 0));
			const uint16_t port = acceptor.local_endpoint().port();

			pClientContext = std::make_shared<asio::io_context>();
			pClientWorkGuard = std::make_shared<asio::executor_work_guard<asio::io_context::executor_type>>(
				asio::make_work_guard(*pClientContext));
			pClientSocket = std::make_shared<asio::ip::tcp::socket>(*pClientContext);
			pClientSocket->connect(asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), port));

			pServerSocket = std::make_shared<asio::ip::tcp::socket>(*pServerContext);
			acceptor.accept(*pServerSocket);
			asio::error_code ec;
			pServerSocket->non_blocking(true, ec);
			REQUIRE_FALSE(ec);

			pSocket = std::make_shared<Socket>(
				SocketAddress::FromEndpoint(pClientSocket->remote_endpoint()),
				pClientContext,
				pClientSocket);
			pSocket->SetOpen(true);

			ioThread = std::thread([context = pClientContext]() { context->run(); });
		}

		~TestSocket()
		{
			pClientWorkGuard.reset();
			pClientContext->stop();
			if (ioThread.joinable()) {
				ioThread.join();
			}
			if (pSocket != nullptr) {
				pSocket->CloseSocket();
				pSocket.reset();
			}
			asio::error_code ec;
			if (pServerSocket != nullptr) {
				pServerSocket->close(ec);
			}
		}
	};
}

// Regression: Socket::SendAsync used to hold the queue only long enough to copy
// a message, so async_write could read a vector that was already destroyed and
// multiple concurrent sends could start overlapping writes. The fix keeps the
// active write buffer alive and serializes queue access.
TEST_CASE("Socket::SendAsync preserves sequential writes")
{
	TestSocket testSocket;

	const std::vector<std::vector<uint8_t>> frames = {
		MakeFrame(1, 64 * 1024, 0xAA),
		MakeFrame(2, 64 * 1024, 0x55),
		MakeFrame(3, 1024, 0x11),
	};
	size_t expectedSize = 0;
	for (const auto& frame : frames) {
		expectedSize += frame.size();
	}
	for (const auto& frame : frames) {
		testSocket.pSocket->SendAsync(frame);
	}

	const std::vector<uint8_t> received = ReadUntil(*testSocket.pServerSocket, expectedSize, std::chrono::seconds(10));

	REQUIRE(received.size() == expectedSize);
	size_t offset = 0;
	for (const auto& frame : frames) {
		const uint32_t length = ReadU32(received, offset);
		const uint32_t id = ReadU32(received, offset + 4);
		REQUIRE(length == frame.size() - 8);
		REQUIRE(id == ReadU32(frame, 4));
		REQUIRE(std::memcmp(received.data() + offset, frame.data(), frame.size()) == 0);
		offset += frame.size();
	}
}

TEST_CASE("Socket::SendAsync serializes concurrent writers")
{
	TestSocket testSocket;

	static constexpr size_t NUM_SENDERS = 4;
	static constexpr size_t FRAMES_PER_SENDER = 4;
	static constexpr size_t PAYLOAD_SIZE = 8 * 1024;
	static constexpr uint32_t FIRST_ID = 100;

	std::vector<std::vector<uint8_t>> frames;
	size_t expectedSize = 0;
	for (size_t sender = 0; sender < NUM_SENDERS; ++sender) {
		for (size_t frame = 0; frame < FRAMES_PER_SENDER; ++frame) {
			const uint32_t id = FIRST_ID + (uint32_t)(sender * FRAMES_PER_SENDER + frame);
			frames.push_back(MakeFrame(id, PAYLOAD_SIZE, (uint8_t)(0x40 + (id & 0x3F))));
			expectedSize += frames.back().size();
		}
	}

	std::vector<std::thread> senders;
	for (size_t sender = 0; sender < NUM_SENDERS; ++sender) {
		senders.emplace_back([&testSocket, &frames, sender]() {
			for (size_t frame = 0; frame < FRAMES_PER_SENDER; ++frame) {
				testSocket.pSocket->SendAsync(frames[sender * FRAMES_PER_SENDER + frame]);
			}
		});
	}

	const std::vector<uint8_t> received = ReadUntil(*testSocket.pServerSocket, expectedSize, std::chrono::seconds(15));
	for (auto& sender : senders) {
		sender.join();
	}

	REQUIRE(received.size() == expectedSize);

	size_t offset = 0;
	std::vector<bool> seen(NUM_SENDERS * FRAMES_PER_SENDER, false);
	size_t frameCount = 0;
	while (offset < received.size()) {
		REQUIRE(received.size() >= offset + 8);
		const uint32_t length = ReadU32(received, offset);
		const uint32_t id = ReadU32(received, offset + 4);
		REQUIRE(length == PAYLOAD_SIZE);
		REQUIRE(id >= FIRST_ID);
		REQUIRE(id < FIRST_ID + NUM_SENDERS * FRAMES_PER_SENDER);
		const size_t index = id - FIRST_ID;
		REQUIRE_FALSE(seen[index]);
		seen[index] = true;

		const uint8_t expectedFill = (uint8_t)(0x40 + (id & 0x3F));
		size_t mismatchedBytes = 0;
		for (size_t byte = 0; byte < length; ++byte) {
			if (received[offset + 8 + byte] != expectedFill) {
				++mismatchedBytes;
			}
		}
		REQUIRE(mismatchedBytes == 0);

		offset += 8 + length;
		++frameCount;
	}

	REQUIRE(frameCount == NUM_SENDERS * FRAMES_PER_SENDER);
	for (size_t index = 0; index < seen.size(); ++index) {
		REQUIRE(seen[index]);
	}
}
