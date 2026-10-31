#pragma once

#include <cstdint>
#include <memory>
#include <atomic>
#include <mutex>
#include <string>

enum class ESyncStatus
{
	SYNCING_HEADERS,
	SYNCING_TXHASHSET,
	PROCESSING_TXHASHSET,
	TXHASHSET_SYNC_FAILED,
	SYNCING_BLOCKS,
	WAITING_FOR_PEERS,
	NOT_SYNCING
};

class SyncStatus
{
public:
	SyncStatus()
		: m_syncStatus(ESyncStatus::SYNCING_HEADERS),
		m_numActiveConnections(0),
		m_networkHeight(0),
		m_networkDifficulty(0),
		m_headerHeight(0),
		m_headerDifficulty(0),
		m_blockHeight(0),
		m_blockDifficulty(0),
		m_lastBlockTime(0),
		m_txHashSetDownloaded(0),
		m_txHashSetTotalSize(0),
		m_txHashSetProcessingStatus(0)
	{

	}
	SyncStatus(const SyncStatus& other)
		: m_syncStatus(other.m_syncStatus.load()),
		m_numActiveConnections(other.m_numActiveConnections.load()),
		m_networkHeight(other.m_networkHeight.load()),
		m_networkDifficulty(other.m_networkDifficulty.load()),
		m_headerHeight(other.m_headerHeight.load()),
		m_headerDifficulty(other.m_headerDifficulty.load()),
		m_blockHeight(other.m_blockHeight.load()),
		m_blockDifficulty(other.m_blockDifficulty.load()),
		m_lastBlockTime(other.m_lastBlockTime.load()),
		m_txHashSetDownloaded(other.m_txHashSetDownloaded.load()),
		m_txHashSetTotalSize(other.m_txHashSetTotalSize.load()),
		m_txHashSetProcessingStatus(other.m_txHashSetProcessingStatus.load()),
		m_blockRequests(other.m_blockRequests.load()),
		m_blockTimeouts(other.m_blockTimeouts.load()),
		m_blockReassignments(other.m_blockReassignments.load()),
		m_bansFraudHeight(other.m_bansFraudHeight.load()),
		m_bansInvalidData(other.m_bansInvalidData.load())
	{
		std::lock_guard<std::mutex> lock(other.m_reasonMutex);
		m_waitingReason = other.m_waitingReason;
	}

	bool IsSyncing() const { return m_syncStatus != ESyncStatus::NOT_SYNCING; }
	ESyncStatus GetStatus() const { return m_syncStatus; }
	uint64_t GetNumActiveConnections() const { return m_numActiveConnections; }
	uint64_t GetNetworkHeight() const { return m_networkHeight; }
	uint64_t GetNetworkDifficulty() const { return m_networkDifficulty; }
	uint64_t GetHeaderHeight() const { return m_headerHeight; }
	uint64_t GetHeaderDifficulty() const { return m_headerDifficulty; }
	uint64_t GetBlockHeight() const { return m_blockHeight; }
	uint64_t GetBlockDifficulty() const { return m_blockDifficulty; }
	time_t GetLastBlockTime() const noexcept { return m_lastBlockTime; }
	uint64_t GetDownloaded() const { return m_txHashSetDownloaded; }
	uint64_t GetDownloadSize() const { return m_txHashSetTotalSize; }
	uint8_t GetProcessingStatus() const { return m_txHashSetProcessingStatus; }

	// Block download observability (REQ_G38_FIX.md §3.8).
	uint64_t GetBlockRequests() const { return m_blockRequests; }
	uint64_t GetBlockTimeouts() const { return m_blockTimeouts; }
	uint64_t GetBlockReassignments() const { return m_blockReassignments; }
	uint64_t GetBansFraudHeight() const { return m_bansFraudHeight; }
	uint64_t GetBansInvalidData() const { return m_bansInvalidData; }
	void IncrementBlockRequests(const uint64_t count = 1) const { m_blockRequests += count; }
	void IncrementBlockTimeouts(const uint64_t count = 1) const { m_blockTimeouts += count; }
	void IncrementBlockReassignments(const uint64_t count = 1) const { m_blockReassignments += count; }
	void IncrementBansFraudHeight(const uint64_t count = 1) const { m_bansFraudHeight += count; }
	void IncrementBansInvalidData(const uint64_t count = 1) const { m_bansInvalidData += count; }

	// Human-readable reason for the WAITING_FOR_PEERS state.
	void SetWaitingReason(const std::string& reason) const
	{
		std::lock_guard<std::mutex> lock(m_reasonMutex);
		m_waitingReason = reason;
	}

	std::string GetWaitingReason() const
	{
		std::lock_guard<std::mutex> lock(m_reasonMutex);
		return m_waitingReason;
	}

	void UpdateStatus(const ESyncStatus syncStatus) { m_syncStatus = syncStatus; }

	void UpdateNetworkStatus(const uint64_t numActiveConnections, const uint64_t networkHeight, const uint64_t networkDifficulty)
	{
		m_numActiveConnections = numActiveConnections;
		if (networkHeight != NULL) m_networkHeight = networkHeight;
		if (networkDifficulty != NULL) m_networkDifficulty = networkDifficulty;
	}

	void UpdateHeaderStatus(const uint64_t headerHeight, const uint64_t headerDifficulty)
	{
		m_headerHeight = headerHeight;
		m_headerDifficulty = headerDifficulty;
	}

	void UpdateBlockStatus(const uint64_t blockHeight, const uint64_t blockDifficulty, const time_t lastBlockTime)
	{
		m_blockHeight = blockHeight;
		m_blockDifficulty = blockDifficulty;
		m_lastBlockTime = lastBlockTime;
	}

	void UpdateDownloaded(const uint64_t downloaded) { m_txHashSetDownloaded = downloaded; }
	void UpdateDownloadSize(const uint64_t downloadSize) { m_txHashSetTotalSize = downloadSize; }
	void UpdateProcessingStatus(const uint8_t processingStatus) { m_txHashSetProcessingStatus = processingStatus; }

private:
	std::atomic<ESyncStatus> m_syncStatus;
	std::atomic<uint64_t> m_numActiveConnections;
	std::atomic<uint64_t> m_networkHeight;
	std::atomic<uint64_t> m_networkDifficulty;
	std::atomic<uint64_t> m_headerHeight;
	std::atomic<uint64_t> m_headerDifficulty;
	std::atomic<uint64_t> m_blockHeight;
	std::atomic<uint64_t> m_blockDifficulty;
	std::atomic<time_t> m_lastBlockTime;
	std::atomic<uint64_t> m_txHashSetDownloaded;
	std::atomic<uint64_t> m_txHashSetTotalSize;
	std::atomic<uint8_t> m_txHashSetProcessingStatus;

	mutable std::atomic<uint64_t> m_blockRequests;
	mutable std::atomic<uint64_t> m_blockTimeouts;
	mutable std::atomic<uint64_t> m_blockReassignments;
	mutable std::atomic<uint64_t> m_bansFraudHeight;
	mutable std::atomic<uint64_t> m_bansInvalidData;
	mutable std::mutex m_reasonMutex;
	mutable std::string m_waitingReason;
};

typedef std::shared_ptr<SyncStatus> SyncStatusPtr;
typedef std::shared_ptr<SyncStatus> SyncStatusConstPtr;