#include "StateSyncer.h"
#include "../Messages/TxHashSetRequestMessage.h"
#include "../PIBD/PibdSync.h"

#include <Consensus.h>
#include <Common/Logger.h>
#include <Core/Global.h>
#include <P2P/Capabilities.h>
#include <P2P/ConnectedPeer.h>
#include <P2P/Peer.h>
#include <P2P/SyncStatus.h>

bool StateSyncer::SyncState(SyncStatus& syncStatus)
{
	if (IsStateSyncDue(syncStatus))
	{
		// Prefer PIBD when a capable peer is connected. On success the state is
		// installed and block sync may proceed; otherwise use txhashset.zip.
		const EPibdResult pibd = TryPibd(syncStatus);
		if (pibd == EPibdResult::InProgress) {
			return true;
		}

		if (pibd == EPibdResult::Completed) {
			return false;
		}

		syncStatus.UpdateStatus(ESyncStatus::SYNCING_TXHASHSET);
		RequestState(syncStatus);

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
		// Logged once when the request is actually sent in RequestState().
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

	if (m_pPeer != nullptr && !m_pConnectionManager.lock()->IsConnected(m_pPeer->GetIPAddress()))
	{
		LOG_WARNING("Sync peer no longer connected.");
		return true;
	}

	return false;
}

bool StateSyncer::RequestState(const SyncStatus& syncStatus)
{
	if (m_pPeer != nullptr)
	{
		LOG_WARNING_F("Banning peer: {}", m_pPeer);
		m_pPeer->Ban(EBanReason::BadTxHashSet);
		m_pPeer = nullptr;
	}

	if (Global::IsRunning())
	{
		if (m_requestedHeight == 0) {
			LOG_INFO("Requesting TxHashSet for the first time.");
		}

		const uint64_t headerHeight = syncStatus.GetHeaderHeight();
		const uint64_t requestedHeight = headerHeight - Consensus::STATE_SYNC_THRESHOLD;
		Hash hash = m_pBlockChain->GetBlockHeaderByHeight(requestedHeight, EChainType::CANDIDATE)->GetHash();

		const TxHashSetRequestMessage txHashSetRequestMessage(std::move(hash), requestedHeight);
		m_pPeer = m_pConnectionManager.lock()->SendMessageToMostWorkPeer(txHashSetRequestMessage);

		if (m_pPeer != nullptr)
		{
			m_timeRequested = std::chrono::system_clock::now();
			m_requestedHeight = requestedHeight;
		}
	}

	return m_pPeer != nullptr;
}

StateSyncer::EPibdResult StateSyncer::TryPibd(SyncStatus& syncStatus)
{
	if (m_pPibdSync == nullptr) {
		return EPibdResult::NotAttempted;
	}

	if (std::chrono::system_clock::now() < m_pibdDisabledUntil) {
		return EPibdResult::NotAttempted;
	}

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	const PibdSync::EStatus status = m_pPibdSync->GetStatus();

	if (status == PibdSync::EStatus::Failed) {
		LOG_WARNING_F("PIBD failed: {}. Falling back to txhashset.zip", m_pPibdSync->GetError());
		m_pPibdSync->Reset();
		DisablePibd();
		return EPibdResult::NotAttempted;
	}

	if (status == PibdSync::EStatus::Idle) {
		if (FindPibdPeer() == nullptr) {
			LOG_DEBUG("No PIBD-capable peer connected; using txhashset.zip");
			return EPibdResult::NotAttempted;
		}

		const BlockHeaderPtr pTip = m_pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE);
		if (pTip == nullptr) {
			return EPibdResult::NotAttempted;
		}

		// PIBD horizon is deterministic and must match the peer segmenter header:
		// archive_height = (header_head - STATE_SYNC_THRESHOLD) rounded down to a
		// multiple of TXHASHSET_ARCHIVE_INTERVAL (grin txhashset_archive_header_header_only).
		uint64_t archiveHeight = pTip->GetHeight();
		archiveHeight = archiveHeight > Consensus::STATE_SYNC_THRESHOLD
			? archiveHeight - Consensus::STATE_SYNC_THRESHOLD
			: 0;
		archiveHeight -= archiveHeight % Consensus::TXHASHSET_ARCHIVE_INTERVAL;

		const BlockHeaderPtr pHorizon =
			m_pBlockChain->GetBlockHeaderByHeight(archiveHeight, EChainType::CANDIDATE);
		if (pHorizon == nullptr) {
			return EPibdResult::NotAttempted;
		}

		PibdDownload::Horizon horizon;
		horizon.BlockHash = pHorizon->GetHash();
		horizon.Height = pHorizon->GetHeight();
		horizon.OutputMmrSize = pHorizon->GetOutputMMRSize();
		horizon.KernelMmrSize = pHorizon->GetKernelMMRSize();
		horizon.OutputRoot = pHorizon->GetOutputRoot();
		horizon.RangeProofRoot = pHorizon->GetRangeProofRoot();
		horizon.KernelRoot = pHorizon->GetKernelRoot();
		m_pibdHorizonHash = horizon.BlockHash;

		if (!m_pPibdSync->Start(horizon, now)) {
			return EPibdResult::NotAttempted;
		}

		LOG_INFO_F(
			"Starting PIBD state sync at height {} (header hash {}), peer {}.",
			horizon.Height,
			horizon.BlockHash,
			FindPibdPeer());
		syncStatus.UpdateStatus(ESyncStatus::SYNCING_TXHASHSET);
		SendPibdRequests(now);
		return EPibdResult::InProgress;
	}

	if (status == PibdSync::EStatus::Downloading) {
		SendPibdRequests(now);

		const PibdSync::EStatus updated = m_pPibdSync->GetStatus();
		if (updated == PibdSync::EStatus::Failed) {
			LOG_WARNING_F("PIBD failed: {}. Falling back to txhashset.zip", m_pPibdSync->GetError());
			m_pPibdSync->Reset();
			DisablePibd();
			return EPibdResult::NotAttempted;
		}

		if (updated != PibdSync::EStatus::Completed) {
			return EPibdResult::InProgress;
		}
	}

	// Completed: release the open txhashset files, write staging, activate
	// atomically and then install and validate the new state.
	m_pBlockChain->CloseTxHashSet();
	if (!m_pPibdSync->TryComplete()) {
		m_pBlockChain->OpenTxHashSet();
		LOG_WARNING_F(
			"PIBD write/activation failed: {}. Falling back to txhashset.zip",
			m_pPibdSync->GetError());
		m_pPibdSync->Reset();
		DisablePibd();
		return EPibdResult::NotAttempted;
	}

	const EBlockChainStatus chainStatus =
		m_pBlockChain->ProcessExistingTxHashSet(m_pibdHorizonHash, syncStatus);
	m_pPibdSync->Reset();

	if (chainStatus == EBlockChainStatus::SUCCESS) {
		LOG_INFO("PIBD state sync completed and validated.");
		return EPibdResult::Completed;
	}

	LOG_WARNING("PIBD state failed chain validation; falling back to txhashset.zip");
	DisablePibd();
	return EPibdResult::NotAttempted;
}

void StateSyncer::DisablePibd()
{
	// Do not immediately retry PIBD; let the legacy state sync have a full attempt.
	m_pibdDisabledUntil = std::chrono::system_clock::now() + std::chrono::minutes(120);
	LOG_INFO("PIBD disabled for 120 minutes after a failure.");
}

void StateSyncer::SendPibdRequests(const std::chrono::steady_clock::time_point now)
{
	if (m_pPibdSync == nullptr) {
		return;
	}

	ConnectionManagerPtr pConnectionManager = m_pConnectionManager.lock();
	if (pConnectionManager == nullptr) {
		return;
	}

	const PeerPtr pPeer = FindPibdPeer();
	if (pPeer == nullptr) {
		return;
	}

	const std::vector<IMessagePtr> requests = m_pPibdSync->TakeRequests(now);
	if (!requests.empty()) {
		LOG_DEBUG_F("Sending {} PIBD request(s) to {}", requests.size(), pPeer);
	}

	for (const IMessagePtr& pMessage : requests) {
		pConnectionManager->SendMessageToPeer(*pMessage, pPeer);
	}
}

PeerPtr StateSyncer::FindPibdPeer() const
{
	ConnectionManagerPtr pConnectionManager = m_pConnectionManager.lock();
	if (pConnectionManager == nullptr) {
		return nullptr;
	}

	PeerPtr pBestPeer = nullptr;
	uint64_t bestHeight = 0;
	for (ConnectedPeer connectedPeer : pConnectionManager->GetConnectedPeers()) {
		PeerPtr pPeer = connectedPeer.GetPeer();
		if (pPeer == nullptr) {
			continue;
		}

		if (!pPeer->GetCapabilities().HasCapability(Capabilities::PIBD_HIST_1)) {
			continue;
		}

		if (pBestPeer == nullptr || connectedPeer.GetHeight() > bestHeight) {
			pBestPeer = pPeer;
			bestHeight = connectedPeer.GetHeight();
		}
	}

	return pBestPeer;
}
