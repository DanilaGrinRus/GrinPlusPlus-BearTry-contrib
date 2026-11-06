#pragma once

#include <asio.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

// Owns the inbound TCP accept loop. It re-arms after every accepted or rejected
// connection, stops cleanly, and backs off after repeated accept errors instead
// of spinning on a broken acceptor.
class AcceptLoop : public std::enable_shared_from_this<AcceptLoop>
{
public:
	using Handler = std::function<void(asio::ip::tcp::socket)>;

	AcceptLoop(asio::io_service& ioService, uint16_t port);
	~AcceptLoop();

	bool Start(Handler handler);

	// Stops accepting new connections and waits for an already-started handler to
	// finish before returning. A handler that has not started yet will not run.
	// Must not be called from inside the handler (self-wait would deadlock).
	void Stop();

	uint16_t GetPort() const;
	bool IsStopped() const { return m_stopped.load(); }
	uint32_t GetConsecutiveErrors() const { return m_consecutiveErrors.load(); }
	static std::chrono::milliseconds RetryDelay(uint32_t consecutiveErrors);

private:
	void DoAccept();
	void HandleAccept(const asio::error_code& ec, const std::shared_ptr<asio::ip::tcp::socket>& pSocket);
	void ScheduleRetry();

	asio::io_service& m_ioService;
	uint16_t m_port;
	Handler m_handler;
	std::shared_ptr<asio::ip::tcp::acceptor> m_pAcceptor;
	std::shared_ptr<asio::steady_timer> m_pRetryTimer;
	std::atomic<bool> m_stopped;
	std::atomic<bool> m_started;
	std::atomic<uint32_t> m_consecutiveErrors;
	mutable std::mutex m_mutex;
	std::condition_variable m_handlerFinished;
	uint32_t m_handlersInFlight = 0;
	std::thread::id m_handlerThread;
};
