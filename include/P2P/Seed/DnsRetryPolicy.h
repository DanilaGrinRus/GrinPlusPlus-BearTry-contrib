#pragma once

#include <chrono>

// DNS peer discovery is retried while no candidate is available. A single
// attempt at startup leaves the node peerless when the seed list happens to be
// stale, so the seeder re-queries DNS on a fixed interval.
namespace DnsRetryPolicy
{
	inline constexpr std::chrono::minutes RETRY_INTERVAL{ 5 };

	inline bool IsDue(
		const std::chrono::system_clock::time_point& lastAttempt,
		const std::chrono::system_clock::time_point& now) noexcept
	{
		return lastAttempt + RETRY_INTERVAL <= now;
	}
}
