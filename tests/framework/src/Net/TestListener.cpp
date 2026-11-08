#include <Net/TestListener.h>

#include <thread>

TestListener::UPtr TestListener::Create(const CreateConnFunc& fn_create_connection, const uint16_t port_number)
{
    std::unique_ptr<TestListener> pListener(new TestListener());

    pListener->m_pAcceptor = std::make_shared<asio::ip::tcp::acceptor>(
        *pListener->m_pAsioContext,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_number)
    );
    pListener->Accept(fn_create_connection);

    return pListener;
}

TestListener::~TestListener()
{
    asio::error_code ec;

    // Stop accepting first so that no new connection can be recorded, cancel the
    // pending async_accept, and take ownership of the connection list.
    std::vector<ITestConnection::Ptr> connections;
    {
        std::unique_lock<std::mutex> lock(m_asioMutex);
        m_stopping = true;

        if (m_pAcceptor) {
            m_pAcceptor->cancel(ec);
            m_pAcceptor->close(ec);
        }

        connections.swap(m_connections);
    }

    // Let every connection cancel its own pending read while the io_context is
    // still running; otherwise Disconnect() would wait forever.
    for (auto& pConnection : connections) {
        pConnection->Disconnect();
    }

    {
        std::unique_lock<std::mutex> lock(m_asioMutex);

        if (m_pSocket) {
            m_pSocket->shutdown(asio::socket_base::shutdown_both, ec);
            m_pSocket->close(ec);
        }

        m_pAsioContext->stop();
    }

    // stop() makes run() return; joining guarantees that no handler is executing
    // and that the io_context is not destroyed while the io thread still uses it.
    if (m_thread.joinable()) {
        m_thread.join();
    }

    {
        std::unique_lock<std::mutex> lock(m_asioMutex);
        // Handlers that were not dispatched before stop() are dropped, so the
        // pending async_accept completion never decrements m_tasks. Nothing is
        // running after join(), therefore this reset is race-free.
        m_pSocket.reset();
        m_pAcceptor.reset();
        m_tasks = 0;
        m_pAsioContext.reset();
    }

    std::cout << "ASIO context destroyed!" << std::endl;
}

void TestListener::Accept(const CreateConnFunc& fn_create_connection)
{
    std::unique_lock<std::mutex> lock(m_asioMutex);
    if (m_stopping || !m_pAsioContext || !m_pAcceptor) {
        return;
    }

    m_tasks++;

    m_pSocket = std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext);
    m_pAcceptor->async_accept(
        *m_pSocket,
        std::bind(&TestListener::OnAccept, this, fn_create_connection, std::placeholders::_1)
    );
}

void TestListener::OnAccept(CreateConnFunc fn_create_connection, const asio::error_code& ec)
{
    if (ec) {
        std::cout << "OnAccept error: " << ec.message() << std::endl;
    } else {
        auto pConnection = fn_create_connection(m_pSocket);
        pConnection->OnInboundConnection();

        {
            std::unique_lock<std::mutex> lock(m_asioMutex);
            if (!m_stopping) {
                m_connections.push_back(pConnection);
            }
        }

        Accept(fn_create_connection);
    }

    m_tasks--;
}
