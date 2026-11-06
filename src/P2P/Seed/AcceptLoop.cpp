#include "AcceptLoop.h"

#include <algorithm>

AcceptLoop::AcceptLoop(asio::io_service& ioService, const uint16_t port)
	: m_ioService(ioService),
	m_port(port),
	m_stopped(true),
	m_started(false),
	m_consecutiveErrors(0)
{

}

AcceptLoop::~AcceptLoop()
{
	Stop();
}

bool AcceptLoop::Start(Handler handler)
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_started.load()) {
			return false;
		}
		m_started.store(true);

		try {
			m_handler = std::move(handler);
			m_pAcceptor = std::make_shared<asio::ip::tcp::acceptor>(m_ioService);
			m_pAcceptor->open(asio::ip::tcp::v4());
			m_pAcceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
			m_pAcceptor->bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), m_port));
			m_pAcceptor->listen(asio::socket_base::max_listen_connections);
			m_port = m_pAcceptor->local_endpoint().port();
			m_stopped.store(false);
			m_consecutiveErrors.store(0);
		}
		catch (const std::exception&) {
			m_stopped.store(true);
			return false;
		}
	}

	DoAccept();
	return true;
}

void AcceptLoop::Stop()
{
	m_stopped.store(true);

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_pRetryTimer != nullptr) {
			asio::error_code ignored;
			m_pRetryTimer->cancel(ignored);
		}

		if (m_pAcceptor != nullptr) {
			asio::error_code ignored;
			m_pAcceptor->close(ignored);
		}
	}

	// An already-started handler is allowed to finish, but Stop() must not return
	// while it can still touch the owner's objects. Handlers not yet admitted see
	// m_stopped and never start.
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_handlerThread == std::this_thread::get_id()) {
		// Stop() from inside the handler cannot wait for itself; callers must not
		// destroy the owner here.
		return;
	}

	m_handlerFinished.wait(lock, [this]() { return m_handlersInFlight == 0; });
}

uint16_t AcceptLoop::GetPort() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_pAcceptor != nullptr && m_pAcceptor->is_open()) {
		asio::error_code ec;
		const auto endpoint = m_pAcceptor->local_endpoint(ec);
		if (!ec) {
			return endpoint.port();
		}
	}

	return m_port;
}

std::chrono::milliseconds AcceptLoop::RetryDelay(const uint32_t consecutiveErrors)
{
	if (consecutiveErrors == 0) {
		return std::chrono::milliseconds(0);
	}

	const uint32_t delay = std::min<uint32_t>(1000, 25 * consecutiveErrors);
	return std::chrono::milliseconds(delay);
}

void AcceptLoop::DoAccept()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_stopped.load() || m_pAcceptor == nullptr || !m_pAcceptor->is_open()) {
		return;
	}

	auto pSocket = std::make_shared<asio::ip::tcp::socket>(m_ioService);
	m_pAcceptor->async_accept(
		*pSocket,
		[self = shared_from_this(), pSocket](const asio::error_code& ec)
		{
			self->HandleAccept(ec, pSocket);
		}
	);
}

void AcceptLoop::HandleAccept(const asio::error_code& ec, const std::shared_ptr<asio::ip::tcp::socket>& pSocket)
{
	if (m_stopped.load()) {
		return;
	}

	if (!ec) {
		Handler handler;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_stopped.load()) {
				return;
			}

			m_consecutiveErrors.store(0);
			handler = m_handler;
			if (handler != nullptr) {
				++m_handlersInFlight;
				m_handlerThread = std::this_thread::get_id();
			}
		}

		if (handler != nullptr) {
			try {
				handler(std::move(*pSocket));
			}
			catch (const std::exception&) {
				// A failing connection handler must not stop the listener.
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				--m_handlersInFlight;
				if (m_handlersInFlight == 0) {
					m_handlerThread = std::thread::id();
				}
			}
			m_handlerFinished.notify_all();
		}

		DoAccept();
		return;
	}

	if (ec == asio::error::operation_aborted) {
		return;
	}

	m_consecutiveErrors.fetch_add(1);
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_stopped.load() || m_pAcceptor == nullptr || !m_pAcceptor->is_open()) {
			return;
		}
	}

	ScheduleRetry();
}

void AcceptLoop::ScheduleRetry()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_stopped.load() || m_pAcceptor == nullptr || !m_pAcceptor->is_open()) {
		return;
	}

	m_pRetryTimer = std::make_shared<asio::steady_timer>(m_ioService);
	m_pRetryTimer->expires_from_now(RetryDelay(m_consecutiveErrors.load()));
	m_pRetryTimer->async_wait(
		[self = shared_from_this()](const asio::error_code& ec)
		{
			if (!ec && !self->m_stopped.load()) {
				self->DoAccept();
			}
		}
	);
}
