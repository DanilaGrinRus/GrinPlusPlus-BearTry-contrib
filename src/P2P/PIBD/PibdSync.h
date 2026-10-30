#pragma once

#include "PibdActivation.h"
#include "PibdScheduler.h"
#include "PibdSnapshot.h"
#include "PibdStateWriter.h"

#include "../Messages/SegmentResponseMessage.h"

#include <Crypto/Models/Hash.h>

#include <filesystem.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Drives one PIBD state sync end to end:
//   download (PibdScheduler) -> snapshot (BuildSnapshot) -> staging write
//   (PibdStateWriter) -> atomic activation (PibdActivation).
//
// The network layer only has to forward the next requests and feed responses
// back; timers are passed in. Any failure leaves the live txhashset untouched and
// reports a status the caller can use to fall back to the legacy txhashset.zip
// download.
class PibdSync
{
public:
	enum class EStatus
	{
		Idle,
		Downloading,
		Completed,     // downloaded and verified, not yet written/activated
		Activated,     // live state replaced
		Failed
	};

	using Clock = std::chrono::steady_clock;

	PibdSync(fs::path liveTxHashSetPath, fs::path stagingDir)
		: m_livePath(std::move(liveTxHashSetPath)),
		m_stagingPath(std::move(stagingDir))
	{
	}

	bool IsActive() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_status == EStatus::Downloading || m_status == EStatus::Completed;
	}

	EStatus GetStatus() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_status;
	}

	std::string GetError() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_error;
	}

	// Starts a download at the given horizon. A stale staging directory from an
	// earlier attempt is removed first.
	bool Start(const PibdDownload::Horizon& horizon, const Clock::time_point now)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_status == EStatus::Downloading || m_status == EStatus::Completed) {
			return false;
		}

		std::error_code error;
		fs::remove_all(m_stagingPath, error);
		fs::create_directories(m_stagingPath, error);

		m_scheduler = std::make_unique<PibdScheduler>(horizon, now);
		m_status = EStatus::Downloading;
		m_error.clear();
		return true;
	}

	// Requests to send now. Empty when idle, complete or failed.
	std::vector<IMessagePtr> TakeRequests(const Clock::time_point now)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_status != EStatus::Downloading || m_scheduler == nullptr) {
			return {};
		}

		if (m_scheduler->IsFailed()) {
			FailLocked(m_scheduler->GetFailure());
			return {};
		}

		if (m_scheduler->IsComplete()) {
			m_status = EStatus::Completed;
			return {};
		}

		return m_scheduler->TakeRequests(now);
	}

	// Feed a segment response. An invalid segment fails the attempt (the caller
	// then falls back to the legacy download).
	bool OnResponse(const SegmentResponseMessage& message, const Clock::time_point now)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_status != EStatus::Downloading || m_scheduler == nullptr) {
			return false;
		}

		try {
			const bool applied = m_scheduler->OnResponse(message, now);
			if (m_scheduler->IsComplete()) {
				m_status = EStatus::Completed;
			}

			return applied;
		}
		catch (const std::exception& e) {
			FailLocked(e.what());
			return false;
		}
	}

	// Write the completed download to staging and activate it. Call only when the
	// status is Completed.
	bool TryComplete()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_status != EStatus::Completed || m_scheduler == nullptr) {
			return false;
		}

		try {
			const PibdSnapshot snapshot =
				BuildSnapshot(m_scheduler->GetSession().GetDownload());
			PibdStateWriter::Write(snapshot, m_stagingPath);
			PibdActivation::Activate(m_stagingPath, m_livePath);
			m_status = EStatus::Activated;
			return true;
		}
		catch (const std::exception& e) {
			FailLocked(e.what());
			return false;
		}
	}

private:
	void FailLocked(const std::string& reason)
	{
		m_status = EStatus::Failed;
		m_error = reason;
	}

	mutable std::mutex m_mutex;
	fs::path m_livePath;
	fs::path m_stagingPath;
	std::unique_ptr<PibdScheduler> m_scheduler;
	EStatus m_status = EStatus::Idle;
	std::string m_error;
};
