#pragma once

#include <Net/TestConnection.h>
#include <asio.hpp>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class TestListener
{
public:
    using UPtr = std::unique_ptr<TestListener>;
    using CreateConnFunc = std::function<ITestConnection::Ptr(const asio_socket_ptr&)>;

    static TestListener::UPtr Create(const CreateConnFunc& fn_create_connection, const uint16_t port_number);
    ~TestListener();

    // Runs the io_context on a joinable thread. The destructor stops the context and
    // joins this thread, so the listener must stay alive until it is destroyed.
    void RunInThread()
    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
        if (m_thread.joinable()) {
            return;
        }

        m_thread = std::thread([this]() { m_pAsioContext->run(); });
    }

private:
    TestListener()
        : m_tasks(0), m_stopping(false), m_pAsioContext(std::make_shared<asio::io_service>()) { }

    void Accept(const CreateConnFunc& fn_create_connection);
    void OnAccept(CreateConnFunc fn_create_connection, const asio::error_code& ec);

    std::mutex m_asioMutex;
    std::atomic_uint32_t m_tasks;
    bool m_stopping;

    std::mutex m_threadMutex;
    std::thread m_thread;

    std::shared_ptr<asio::io_service> m_pAsioContext;
    std::shared_ptr<asio::ip::tcp::acceptor> m_pAcceptor;
    asio_socket_ptr m_pSocket;

    std::vector<ITestConnection::Ptr> m_connections;
};
