#include "StateSyncer.h"
#include "../Messages/TxHashSetRequestMessage.h"

#include <Consensus.h>
#include <Common/Logger.h>
#include <Core/Global.h>

StateSyncer::StateSyncer(Dependencies dependencies)
	: m_timeRequested(std::chrono::system_clock::now()),
	m_requestedHeight(0),
	m_pPeer(nullptr),
	m_hasExcludedPeer(false),
	m_dependencies(std::move(dependencies))
{

}

void StateSyncer::SetLastRequestForTest(const uint64_t requestedHeight, const std::chrono::system_clock::time_point timeRequested)
{
	m_requestedHeight = requestedHeight;
	m_timeRequested = timeRequested;
}

void StateSyncer::SetPeerForTest(const PeerPtr peer)
{
	m_pPeer = peer;
}

bool StateSyncer::SyncState(SyncStatus& syncStatus)
{
	if (IsStateSyncDue(syncStatus))
	{
		if (RequestState(syncStatus))
		{
			syncStatus.UpdateStatus(ESyncStatus::SYNCING_TXHASHSET);
		}
		else
		{
			// No request went out: do not keep claiming that txhashset sync is in progress.
			syncStatus.UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		}

		return true;
	}

	// If state sync is still in progress, return true to delay block sync.
	if (syncStatus.GetBlockHeight() < (syncStatus.GetHeaderHeight() - Consensus::CUT_THROUGH_HORIZON))
	{
		return true;
	}

	return false;
}

// NOTE: This doesn't handle re-orgs beyond the horizon.
bool StateSyncer::IsStateSyncDue(const SyncStatus& syncStatus) const
{
	const uint64_t headerHeight = syncStatus.GetHeaderHeight();
	const uint64_t blockHeight = syncStatus.GetBlockHeight();

	const ESyncStatus status = syncStatus.GetStatus();
	if (status == ESyncStatus::PROCESSING_TXHASHSET)
	{
		return false;
	}

	if (status == ESyncStatus::TXHASHSET_SYNC_FAILED)
	{
		LOG_WARNING("TxHashSet sync failed.");
		return true;
	}

	// For the first week, there's no reason to request TxHashSet, since we can just download full blocks.
	if (headerHeight < Consensus::CUT_THROUGH_HORIZON)
	{
		return false;
	}

	// If block height is within threshold, just rely on block sync.
	if (blockHeight > (headerHeight - Consensus::CUT_THROUGH_HORIZON))
	{
		return false;
	}

	if (m_requestedHeight == 0)
	{
		LOG_INFO("Requesting TxHashSet for the first time.");
		return true;
	}

	// If TxHashSet download timed out, request it from another peer.
	if ((m_timeRequested + std::chrono::minutes(120)) < std::chrono::system_clock::now())
	{
		LOG_WARNING("Download timed out (120 minutes).");
		return true;
	}

	// If 10 seconds elapsed with no progress, try another peer.
	if ((m_timeRequested + std::chrono::seconds(10)) < std::chrono::system_clock::now())
	{
		const uint64_t downloaded = syncStatus.GetDownloaded();
		if (downloaded == 0)
		{
			LOG_WARNING("10 seconds elapsed and download still not started.");
			return true;
		}
	}

	if (m_pPeer != nullptr && !IsConnected(m_pPeer->GetIPAddress()))
	{
		LOG_WARNING("Sync peer no longer connected.");
		return true;
	}

	return false;
}

bool StateSyncer::RequestState(const SyncStatus& syncStatus)
{
	// A slow or unresponsive peer is rotated, never banned: a real bad
	// txhashset is detected and banned by the txhashset pipeline itself.
	if (Global::IsRunning())
	{
		PeerPtr next = ChooseAlternativePeer(
			m_pPeer,
			m_dependencies.has_value() ? m_dependencies->getMostWorkPeers() : m_pConnectionManager.lock()->GetMostWorkPeers()
		);

		const uint64_t headerHeight = syncStatus.GetHeaderHeight();
		const uint64_t requestedHeight = headerHeight - Consensus::STATE_SYNC_THRESHOLD;
		Hash hash = m_dependencies.has_value()
			? m_dependencies->getHeaderHash(requestedHeight)
			: m_pBlockChain->GetBlockHeaderByHeight(requestedHeight, EChainType::CANDIDATE)->GetHash();

		const TxHashSetRequestMessage txHashSetRequestMessage(std::move(hash), requestedHeight);
		const bool sent = m_dependencies.has_value()
			? (next != nullptr && m_dependencies->sendMessageToPeer(txHashSetRequestMessage, next))
			: (next != nullptr && m_pConnectionManager.lock()->SendMessageToPeer(txHashSetRequestMessage, next));
		if (sent)
		{
			m_pPeer = next;
			m_timeRequested = std::chrono::system_clock::now();
			m_requestedHeight = requestedHeight;
		}
		else
		{
			// No alternative: leave m_pPeer null and retry on the next sync tick.
			LOG_WARNING("No alternative txhashset peer available; will retry.");
		}
	}

	return m_pPeer != nullptr;
}

void StateSyncer::RotatePeer(PeerPtr& peer)
{
	if (peer != nullptr)
	{
		LOG_INFO_F("Rotating txhashset sync peer: {}", peer);
		peer = nullptr;
	}
}

PeerPtr StateSyncer::SelectAlternativePeer(const std::vector<PeerPtr>& peers, const IPAddress& previous, const bool hasPrevious)
{
	for (const PeerPtr& peer : peers)
	{
		if (peer == nullptr || peer->IsBanned())
		{
			continue;
		}

		if (hasPrevious && peer->GetIPAddress() == previous)
		{
			continue;
		}

		return peer;
	}

	return nullptr;
}

PeerPtr StateSyncer::ChooseAlternativePeer(PeerPtr& current, const std::vector<PeerPtr>& peers)
{
	if (current != nullptr)
	{
		m_excludedPeer = current->GetIPAddress();
		m_hasExcludedPeer = true;
	}

	RotatePeer(current);

	return SelectAlternativePeer(peers, m_excludedPeer, m_hasExcludedPeer);
}

bool StateSyncer::IsConnected(const IPAddress& address) const
{
	if (m_dependencies.has_value())
	{
		return m_dependencies->isConnected(address);
	}

	return m_pConnectionManager.lock()->IsConnected(address);
}
