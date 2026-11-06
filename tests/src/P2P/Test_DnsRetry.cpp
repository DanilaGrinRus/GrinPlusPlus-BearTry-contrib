#include <catch.hpp>

#include <P2P/Seed/DnsRetryPolicy.h>

#include <chrono>

// The node must not become peerless forever when the seed list is stale at
// startup: DNS is queried once, then retried on a five minute interval while
// no candidate is available. These cases pin that policy without network I/O.
TEST_CASE("DnsRetryPolicy::IsDue")
{
	using clock = std::chrono::system_clock;
	const auto epoch = clock::time_point{};

	// First lookup in a fresh process: nothing attempted yet.
	REQUIRE(DnsRetryPolicy::IsDue(clock::time_point::min(), epoch));

	// No retry before the five minute interval.
	REQUIRE_FALSE(DnsRetryPolicy::IsDue(epoch, epoch + std::chrono::minutes(4)));
	REQUIRE_FALSE(DnsRetryPolicy::IsDue(epoch, epoch + std::chrono::minutes(5) - std::chrono::seconds(1)));

	// Retry at and after the interval.
	REQUIRE(DnsRetryPolicy::IsDue(epoch, epoch + std::chrono::minutes(5)));
	REQUIRE(DnsRetryPolicy::IsDue(epoch, epoch + std::chrono::minutes(6)));

	// A backwards clock jump must not trigger a retry storm.
	REQUIRE_FALSE(DnsRetryPolicy::IsDue(epoch + std::chrono::minutes(10), epoch));
}
