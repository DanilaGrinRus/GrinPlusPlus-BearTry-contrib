#include "Seeder.h"
#include "AcceptLoop.h"
#include "DNSSeeder.h"
#include "PeerManager.h"
#include "../ConnectionManager.h"
#include "../Messages/GetPeerAddressesMessage.h"

#include <Core/Context.h>
#include <P2P/Capabilities.h>
#include <Net/SocketAddress.h>
#include <Net/Socket.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <functional>
#include <algorithm>

Seeder::~Seeder()
{
    LOG_INFO("Shutting down seeder");
    m_terminate = true;
    StopListener();

    m_pAsioContext->stop();

    std::unique_lock<std::mutex> lock(m_mutex);
    ThreadUtil::Join(m_seedThread);
    ThreadUtil::Join(m_asioThread);
}

std::unique_ptr<Seeder> Seeder::Create(
    ConnectionManager& connectionManager,
    Locked<PeerManager> peerManager,
    const IBlockChain::Ptr& pBlockChain,
    std::shared_ptr<Pipeline> pPipeline,
    SyncStatusConstPtr pSyncStatus)
{
    auto pMessageProcessor = std::make_shared<MessageProcessor>(
        connectionManager,
        peerManager,
        pBlockChain,
        pPipeline,
        pSyncStatus
    );
    std::unique_ptr<Seeder> pSeeder(new Seeder(
        connectionManager,
        peerManager,
        pBlockChain,
        pMessageProcessor,
        pSyncStatus
    ));

    pSeeder->m_asioThread = std::thread(Thread_AsioContext, std::ref(*pSeeder.get()));
    pSeeder->m_seedThread = std::thread(Thread_Seed, std::ref(*pSeeder.get()));

    return pSeeder;
}

void Seeder::Thread_AsioContext(Seeder& seeder)
{
    LoggerAPI::SetThreadName("ASIO");

    seeder.StartListener();

    while (!seeder.m_terminate) {
        asio::error_code ec;
        seeder.m_pAsioContext->run(ec);
    }
}

//
// Continuously checks the number of connected peers, and connects to additional peers when the number of connections drops below the minimum.
// This function operates in its own thread.
//
void Seeder::Thread_Seed(Seeder& seeder)
{
    LoggerAPI::SetThreadName("SEED");
    LOG_TRACE("BEGIN");

    auto lastConnectTime = std::chrono::system_clock::now() - std::chrono::seconds(5);

    const size_t minimumConnections = Global::GetConfig().GetMinPeers();
    while (!seeder.m_terminate && Global::IsRunning()) {
        try {
            seeder.m_connectionManager.PruneConnections(true);

            auto now = std::chrono::system_clock::now();
            const size_t numOutbound = seeder.m_connectionManager.GetNumOutbound();
            if (numOutbound < minimumConnections && lastConnectTime + std::chrono::seconds(5) < now) {
                lastConnectTime = now;

                const size_t connectionsToAdd = (std::min)((size_t)15, (minimumConnections - numOutbound));
                for (size_t i = 0; i < connectionsToAdd; i++) {
                    seeder.SeedNewConnection();
                }
            }
        }
        catch (std::exception& e) {
            LOG_WARNING_F("Exception thrown: {}", e.what());
        }

        ThreadUtil::SleepFor(std::chrono::milliseconds(100));
    }

    LOG_TRACE("END");
}

void Seeder::StartListener()
{
    m_pAcceptLoop = std::make_shared<AcceptLoop>(
        *m_pAsioContext,
        Global::GetConfig().GetP2PPort()
    );

    if (!m_pAcceptLoop->Start(
        [this](asio::ip::tcp::socket socket)
        {
            OnAccept(std::move(socket));
        }))
    {
        LOG_ERROR("Listener failed to start.");
    }
}

void Seeder::StopListener()
{
    if (m_pAcceptLoop != nullptr)
    {
        m_pAcceptLoop->Stop();
        m_pAcceptLoop = nullptr;
    }
}

void Seeder::OnAccept(asio::ip::tcp::socket socket)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    asio::error_code endpointError;
    const auto endpoint = socket.remote_endpoint(endpointError);
    if (endpointError)
    {
        asio::error_code ignored;
        socket.close(ignored);
        return;
    }

    SocketPtr pSocket(new Socket(
        SocketAddress::FromEndpoint(endpoint),
        m_pAsioContext,
        std::make_shared<asio::ip::tcp::socket>(std::move(socket))
    ));
    pSocket->SetOpen(true);

    const uint64_t activeConnections = m_connectionManager.GetNumberOfActiveConnections();
    const uint64_t maxPeers = Global::GetConfig().GetMaxPeers();
    const IPAddress& ipAddress = pSocket->GetIPAddress();
    const bool blocked = Global::GetConfig().IsPeerBlocked(ipAddress);
    const bool allowed = !blocked && Global::GetConfig().IsPeerAllowed(ipAddress);
    PeerPtr pPeer = nullptr;
    if (allowed)
    {
        pPeer = m_peerManager.Write()->GetPeer(ipAddress);
    }
    const bool banned = (pPeer != nullptr) && pPeer->IsBanned();

    if (ShouldAcceptPeer(false, activeConnections, maxPeers, blocked, allowed, banned))
    {
        auto pConnection = std::make_shared<Connection>(
            pSocket,
            m_nextId++,
            m_connectionManager,
            ConnectedPeer(pPeer, EDirection::INBOUND, pSocket->GetPort()),
            m_pSyncStatus,
            m_pMessageProcessor
        );
        pConnection->Connect();
    }
    else
    {
        if (blocked)
        {
            LOG_TRACE_F("peer is blocked: {}", ipAddress);
        }
        else if (!allowed)
        {
            LOG_TRACE_F("peer is not allowed: {}", ipAddress);
        }
        else if (banned)
        {
            LOG_TRACE_F("peer is banned: {}", ipAddress);
        }

        pSocket->CloseSocket();
    }
}

void Seeder::SeedNewConnection()
{
    PeerPtr pPeer = m_peerManager.Write()->GetNewPeer(Capabilities::FAST_SYNC_NODE);
    if (pPeer != nullptr) {
        LOG_TRACE_F("Attempting to seed: {}", pPeer);

        ConnectedPeer connectedPeer(
            pPeer,
            EDirection::OUTBOUND,
            Global::GetConfig().GetP2PPort()
        );
        SocketPtr pSocket(new Socket(
            connectedPeer.GetSocketAddress(),
            m_pAsioContext,
            std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext)
        ));
        
        if (Global::GetConfig().IsPeerBlocked(pSocket->GetIPAddress())) {
            LOG_TRACE_F("peer is blocked: {}", pSocket->GetIPAddress());
            return;
        }

        if (!Global::GetConfig().IsPeerAllowed(pSocket->GetIPAddress())) {
            LOG_TRACE_F("peer is not allowed: {}", pSocket->GetIPAddress());        
            return;
        }

        if (!Global::GetConfig().IsPeerPreferred(pSocket->GetIPAddress())) {
            LOG_TRACE_F("peer is not in preferred pool of peers: {}", pSocket->GetIPAddress());        
            return;
        }
        
        ConnectionPtr pConnection = std::make_shared<Connection>(
            pSocket,
            m_nextId++,
            m_connectionManager,
            connectedPeer,
            m_pSyncStatus,
            m_pMessageProcessor
        );
        
        pConnection->Connect();
    } else if (!m_usedDNS.exchange(true)) {
        std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();
        m_peerManager.Write()->AddFreshPeers(peerAddresses);
    }
}

bool Seeder::ShouldAcceptPeer(
	bool hasError,
	uint64_t activeConnections,
	uint64_t maxPeers,
	bool blocked,
	bool allowed,
	bool banned)
{
	return !hasError && activeConnections < maxPeers && !blocked && allowed && !banned;
}
