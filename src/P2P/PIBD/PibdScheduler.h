#pragma once

#include "PibdDownload.h"
#include "PibdParams.h"
#include "PibdSession.h"

#include "../Messages/SegmentResponseMessage.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Drives a PIBD session over time: issues requests up to an in-flight limit,
// retries segments that time out, and gives up (for the legacy txhashset.zip
// fallback) after too many attempts or too long without progress.
//
// Time is injected so the retry/timeout behaviour is deterministic in tests; the
// network layer passes its own clock.
class PibdScheduler
{
public:
	using Clock = std::chrono::steady_clock;

	struct Config
	{
		size_t MaxInFlight = PibdParams::SEGMENT_REQUEST_COUNT;
		std::chrono::seconds Timeout = PibdParams::SEGMENT_REQUEST_TIMEOUT;
		size_t MaxAttempts = 3;
		std::chrono::seconds FallbackAfter = PibdParams::TXHASHSET_ZIP_FALLBACK_TIME;
	};

	PibdScheduler(PibdDownload::Horizon horizon, const Clock::time_point now, Config config = Config())
		: m_session(std::move(horizon)),
		m_config(config),
		m_lastProgress(now)
	{
	}

	const PibdSession& GetSession() const noexcept { return m_session; }
	PibdSession& GetSession() noexcept { return m_session; }

	bool IsComplete() const noexcept { return m_session.GetDownload().IsComplete(); }
	bool IsFailed() const noexcept { return m_failed; }
	const std::string& GetFailure() const noexcept { return m_failure; }
	size_t GetInFlightCount() const noexcept { return m_inFlight.size(); }
	size_t GetTotalRequestsSent() const noexcept { return m_requestsSent; }

	// Expire timed-out requests and return the messages to send now.
	std::vector<IMessagePtr> TakeRequests(const Clock::time_point now)
	{
		if (m_failed || IsComplete()) {
			return {};
		}

		Expire(now);
		if (m_failed) {
			return {};
		}

		const size_t capacity = (m_config.MaxInFlight > m_inFlight.size())
			? m_config.MaxInFlight - m_inFlight.size()
			: 0;
		if (capacity == 0) {
			return {};
		}

		// Ask the session for more candidates than we can send, then drop the ones
		// that are already in flight.
		const std::vector<PibdDownload::SegmentRequest> candidates =
			m_session.GetDownload().NextRequests(capacity + m_inFlight.size());
		std::vector<IMessagePtr> messages;
		for (const PibdDownload::SegmentRequest& candidate : candidates) {
			if (messages.size() >= capacity) {
				break;
			}

			if (IsInFlight(candidate)) {
				continue;
			}

			const uint32_t attempts = m_attempts[Key(candidate)] + 1;
			if (attempts > m_config.MaxAttempts) {
				Fail("segment " + Describe(candidate) + " exceeded the retry limit");
				return messages;
			}

			m_attempts[Key(candidate)] = attempts;
			m_inFlight.push_back(Pending{ candidate, now + m_config.Timeout });
			messages.push_back(m_session.CreateRequest(candidate));
			++m_requestsSent;
		}

		return messages;
	}

	// Feed a segment response. Returns false when the response is stale/duplicate
	// or for another horizon; throws SegmentException for an invalid payload.
	bool OnResponse(const SegmentResponseMessage& message, const Clock::time_point now)
	{
		const std::optional<PibdDownload::SegmentRequest> request =
			PibdSession::ResponseRequest(message);
		if (!request.has_value()) {
			throw SegmentException(ESegmentError::Mismatch, "Message is not a segment response");
		}

		const bool applied = m_session.AcceptResponse(message);
		if (applied) {
			RemoveInFlight(request.value());
			m_lastProgress = now;
		}

		return applied;
	}

private:
	struct KeyValue
	{
		int Tree;
		uint8_t Height;
		uint64_t Idx;

		bool operator<(const KeyValue& rhs) const noexcept
		{
			if (Tree != rhs.Tree) {
				return Tree < rhs.Tree;
			}

			if (Height != rhs.Height) {
				return Height < rhs.Height;
			}

			return Idx < rhs.Idx;
		}
	};

	struct Pending
	{
		PibdDownload::SegmentRequest Request;
		Clock::time_point Deadline;
	};

	static KeyValue Key(const PibdDownload::SegmentRequest& request)
	{
		return KeyValue{
			(int)request.Tree,
			request.Identifier.GetHeight(),
			request.Identifier.GetIdx()
		};
	}

	static std::string Describe(const PibdDownload::SegmentRequest& request)
	{
		return "tree " + std::to_string((int)request.Tree) + " height " +
			std::to_string((int)request.Identifier.GetHeight()) + " idx " +
			std::to_string(request.Identifier.GetIdx());
	}

	bool IsInFlight(const PibdDownload::SegmentRequest& request) const
	{
		for (const Pending& pending : m_inFlight) {
			if (pending.Request == request) {
				return true;
			}
		}

		return false;
	}

	void RemoveInFlight(const PibdDownload::SegmentRequest& request)
	{
		for (auto iter = m_inFlight.begin(); iter != m_inFlight.end(); ++iter) {
			if (iter->Request == request) {
				m_inFlight.erase(iter);
				return;
			}
		}
	}

	void Expire(const Clock::time_point now)
	{
		for (auto iter = m_inFlight.begin(); iter != m_inFlight.end();) {
			if (iter->Deadline <= now) {
				iter = m_inFlight.erase(iter);
			} else {
				++iter;
			}
		}

		if (m_lastProgress + m_config.FallbackAfter <= now) {
			Fail("no PIBD progress within the fallback window");
		}
	}

	void Fail(const std::string& reason)
	{
		if (!m_failed) {
			m_failed = true;
			m_failure = reason;
		}
	}

	PibdSession m_session;
	Config m_config;
	std::map<KeyValue, uint32_t> m_attempts;
	std::vector<Pending> m_inFlight;
	Clock::time_point m_lastProgress;
	size_t m_requestsSent = 0;
	bool m_failed = false;
	std::string m_failure;
};
