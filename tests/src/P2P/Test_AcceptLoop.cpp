#include <catch.hpp>

#include <P2P/Seed/AcceptLoop.h>

#include <asio.hpp>

#include <atomic>
#include <chrono>
#include <thread>

namespace {

bool CanConnect(const uint16_t port)
{
    asio::io_context io;
    asio::ip::tcp::socket socket(io);
    asio::steady_timer timer(io);
    bool connected = false;

    socket.async_connect(
        asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), port),
        [&connected, &timer](const asio::error_code& ec)
        {
            connected = !ec;
            asio::error_code ignored;
            timer.cancel(ignored);
        }
    );
    timer.expires_after(std::chrono::milliseconds(500));
    timer.async_wait([&socket](const asio::error_code& ec)
    {
        if (!ec) {
            asio::error_code ignored;
            socket.close(ignored);
        }
    });

    io.run();
    return connected;
}

bool WaitFor(const std::atomic<int>& value, const int expected)
{
    for (int i = 0; i < 200; ++i) {
        if (value.load() >= expected) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return value.load() >= expected;
}

}

TEST_CASE("Accept loop accepts again after a rejected peer")
{
    asio::io_service io;
    auto loop = std::make_shared<AcceptLoop>(io, 0);
    std::atomic<int> accepted{ 0 };

    REQUIRE(loop->Start([&accepted](asio::ip::tcp::socket socket)
    {
        // Reject the connection: the loop must still be armed for the next peer.
        ++accepted;
        asio::error_code ignored;
        socket.close(ignored);
    }));

    const uint16_t port = loop->GetPort();
    std::thread ioThread([&io]() { io.run(); });

    REQUIRE(CanConnect(port));
    REQUIRE(WaitFor(accepted, 1));

    REQUIRE(CanConnect(port));
    REQUIRE(WaitFor(accepted, 2));

    loop->Stop();
    const int afterStop = accepted.load();
    REQUIRE(loop->IsStopped());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(accepted.load() == afterStop);

    io.stop();
    ioThread.join();
}

TEST_CASE("Accept loop applies backoff for repeated errors")
{
    REQUIRE(AcceptLoop::RetryDelay(0) == std::chrono::milliseconds(0));
    REQUIRE(AcceptLoop::RetryDelay(1) == std::chrono::milliseconds(25));
    REQUIRE(AcceptLoop::RetryDelay(4) == std::chrono::milliseconds(100));
    REQUIRE(AcceptLoop::RetryDelay(100) == std::chrono::milliseconds(1000));
}

TEST_CASE("Accept loop stop is idempotent and start fails after stop")
{
    asio::io_service io;
    auto loop = std::make_shared<AcceptLoop>(io, 0);
    REQUIRE(loop->Start([](asio::ip::tcp::socket socket) {
        asio::error_code ignored;
        socket.close(ignored);
    }));
    loop->Stop();
    loop->Stop();
    REQUIRE_FALSE(loop->Start([](asio::ip::tcp::socket socket) {
        asio::error_code ignored;
        socket.close(ignored);
    }));
}

TEST_CASE("Accept loop concurrent Stop is safe")
{
    asio::io_service io;
    auto loop = std::make_shared<AcceptLoop>(io, 0);
    std::atomic<int> accepted{ 0 };

    REQUIRE(loop->Start([&accepted](asio::ip::tcp::socket socket)
    {
        ++accepted;
        asio::error_code ignored;
        socket.close(ignored);
    }));

    std::thread ioThread([&io]() { io.run(); });

    std::vector<std::thread> stoppers;
    for (int i = 0; i < 4; ++i) {
        stoppers.emplace_back([loop]() { loop->Stop(); });
    }
    for (auto& stopper : stoppers) {
        stopper.join();
    }

    REQUIRE(loop->IsStopped());

    io.stop();
    ioThread.join();
}

TEST_CASE("Accept loop Stop waits for an in-flight handler")
{
    asio::io_service io;
    auto loop = std::make_shared<AcceptLoop>(io, 0);

    std::mutex gateMutex;
    std::condition_variable gate;
    bool entered = false;
    bool release = false;

    REQUIRE(loop->Start([&](asio::ip::tcp::socket socket)
    {
        {
            std::lock_guard<std::mutex> lock(gateMutex);
            entered = true;
        }
        gate.notify_all();

        std::unique_lock<std::mutex> lock(gateMutex);
        gate.wait(lock, [&release]() { return release; });

        asio::error_code ignored;
        socket.close(ignored);
    }));

    const uint16_t port = loop->GetPort();
    std::thread ioThread([&io]() { io.run(); });

    REQUIRE(CanConnect(port));
    {
        std::unique_lock<std::mutex> lock(gateMutex);
        REQUIRE(gate.wait_for(lock, std::chrono::seconds(5), [&entered]() { return entered; }));
    }

    std::atomic<bool> stopReturned{ false };
    std::thread stopper([&]() {
        loop->Stop();
        stopReturned.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE_FALSE(stopReturned.load());

    {
        std::lock_guard<std::mutex> lock(gateMutex);
        release = true;
    }
    gate.notify_all();

    stopper.join();
    REQUIRE(stopReturned.load());

    io.stop();
    ioThread.join();
}
