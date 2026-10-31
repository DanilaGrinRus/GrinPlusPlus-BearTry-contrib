#pragma once

#include "../ConnectionManager.h"

#include <BlockChain/BlockChain.h>
#include <chrono>
#include <memory>

// Forward Declarations
class SyncStatus;
class PibdSync;

class StateSyncer
{
public:
	StateSyncer(
		const std::weak_ptr<ConnectionManager>& pConnectionManager,
		const IBlockChain::Ptr& pBlockChain,
		std::shared_ptr<PibdSync> pPibdSync = nullptr
	)
		: m_pConnectionManager(pConnectionManager),
		m_pBlockChain(pBlockChain),
		m_pPibdSync(std::move(pPibdSync))
	{
		m_timeRequested = std::chrono::system_clock::now();
		m_requestedHeight = 0;
		m_pPeer = nullptr;
	}

	bool SyncState(SyncStatus& syncStatus);

	enum class EPibdResult
	{
		NotAttempted,   // fall back to txhashset.zip
		InProgress,     // keep waiting
		Completed       // state installed, block sync can proceed
	};

private:
	bool IsStateSyncDue(const SyncStatus& syncStatus) const;
	bool RequestState(const SyncStatus& syncStatus);

	// PIBD path: starts the segment download, feeds requests to a capable peer and
	// installs the state once it has been written and activated.
	// Returns true while PIBD is in progress, false to fall back to txhashset.zip.
	EPibdResult TryPibd(SyncStatus& syncStatus);
	void DisablePibd();
	void SendPibdRequests(const std::chrono::steady_clock::time_point now);
	PeerPtr FindPibdPeer() const;

	std::chrono::time_point<std::chrono::system_clock> m_timeRequested;
	uint64_t m_requestedHeight;
	PeerPtr m_pPeer;

	std::weak_ptr<ConnectionManager> m_pConnectionManager;
	IBlockChain::Ptr m_pBlockChain;
	std::shared_ptr<PibdSync> m_pPibdSync;
	std::chrono::system_clock::time_point m_pibdDisabledUntil = std::chrono::system_clock::time_point::min();
	Hash m_pibdHorizonHash;
};
