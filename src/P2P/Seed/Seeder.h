#pragma once

#include "../ConnectionManager.h"
#include "../MessageProcessor.h"

#include <Core/Config.h>
#include <BlockChain/BlockChain.h>
#include <P2P/Direction.h>
#include <cstdint>
#include <memory>
#include <atomic>
#include <thread>
#include <optional>
#include <asio.hpp>

// Forward Declarations
class PeerManager;
class Pipeline;
class AcceptLoop;

class Seeder
{
public:
	static std::unique_ptr<Seeder> Create(
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		const IBlockChain::Ptr& pBlockChain,
		std::shared_ptr<Pipeline> pPipeline,
		SyncStatusConstPtr pSyncStatus
	);
	~Seeder();

	// Pure decision used by Accept; exposed for the regression test.
	static bool ShouldAcceptPeer(bool hasError, uint64_t activeConnections, uint64_t maxPeers,
		bool blocked, bool allowed, bool banned);

private:
	Seeder(
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		const IBlockChain::Ptr& pBlockChain,
		std::shared_ptr<MessageProcessor> pMessageProcessor,
		SyncStatusConstPtr pSyncStatus
	) : m_connectionManager(connectionManager),
		m_peerManager(peerManager),
		m_pBlockChain(pBlockChain),
		m_pMessageProcessor(pMessageProcessor),
		m_pSyncStatus(pSyncStatus),
		m_pAsioContext(std::make_shared<asio::io_service>()),
		m_terminate(false) { }

	static void Thread_AsioContext(Seeder& seeder);
	static void Thread_Seed(Seeder& seeder);
	void StartListener();
	void StopListener();

	void OnAccept(asio::ip::tcp::socket socket);
	void SeedNewConnection();

	ConnectionManager& m_connectionManager;
	Locked<PeerManager> m_peerManager;
	IBlockChain::Ptr m_pBlockChain;
	std::shared_ptr<MessageProcessor> m_pMessageProcessor;
	SyncStatusConstPtr m_pSyncStatus;

	std::atomic<bool> m_terminate = true;

	std::shared_ptr<asio::io_service> m_pAsioContext;
	std::shared_ptr<AcceptLoop> m_pAcceptLoop;

	std::thread m_asioThread;
	std::thread m_seedThread;

	std::mutex m_mutex;
	mutable std::atomic_bool m_usedDNS = false;
	mutable uint64_t m_nextId = { 1 };
};