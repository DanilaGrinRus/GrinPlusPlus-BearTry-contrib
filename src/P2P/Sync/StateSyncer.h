#pragma once

#include "../ConnectionManager.h"

#include <BlockChain/BlockChain.h>
#include <Net/IPAddress.h>
#include <chrono>
#include <functional>
#include <optional>
#include <vector>

// Forward Declarations
class SyncStatus;

class StateSyncer
{
public:
	// Test seams: production uses ConnectionManager/BlockChain directly.
	struct Dependencies
	{
		std::function<std::vector<PeerPtr>()> getMostWorkPeers;
		std::function<bool(const IMessage&, PeerConstPtr)> sendMessageToPeer;
		std::function<bool(const IPAddress&)> isConnected;
		std::function<Hash(uint64_t)> getHeaderHash;
	};

	explicit StateSyncer(Dependencies dependencies);

	StateSyncer(const std::weak_ptr<ConnectionManager>& pConnectionManager, const IBlockChain::Ptr& pBlockChain)
		: m_pConnectionManager(pConnectionManager), m_pBlockChain(pBlockChain)
	{
		m_timeRequested = std::chrono::system_clock::now();
		m_requestedHeight = 0;
		m_pPeer = nullptr;
		m_hasExcludedPeer = false;
	}

	bool SyncState(SyncStatus& syncStatus);

	// Test-only state injection; does not change production behaviour.
	void SetLastRequestForTest(uint64_t requestedHeight, std::chrono::system_clock::time_point timeRequested);
	void SetPeerForTest(PeerPtr peer);

	// Clears the sync peer without banning it; slow peers are rotated, not punished.
	static void RotatePeer(PeerPtr& peer);

	// Picks a non-banned most-work peer that is not the peer we are rotating away from.
	static PeerPtr SelectAlternativePeer(const std::vector<PeerPtr>& peers, const IPAddress& previous, bool hasPrevious);

	// Rotates the current peer and remembers its address across retries, so a failed
	// attempt without an alternative cannot silently select the same slow peer again.
	PeerPtr ChooseAlternativePeer(PeerPtr& current, const std::vector<PeerPtr>& peers);

private:
	bool IsStateSyncDue(const SyncStatus& syncStatus) const;
	bool RequestState(const SyncStatus& syncStatus);
	bool IsConnected(const IPAddress& address) const;

	std::chrono::time_point<std::chrono::system_clock> m_timeRequested;
	uint64_t m_requestedHeight;
	PeerPtr m_pPeer;
	IPAddress m_excludedPeer;
	bool m_hasExcludedPeer;

	std::weak_ptr<ConnectionManager> m_pConnectionManager;
	IBlockChain::Ptr m_pBlockChain;
	std::optional<Dependencies> m_dependencies;
};