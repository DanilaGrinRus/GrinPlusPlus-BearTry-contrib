#include <catch.hpp>

#include <P2P/Capabilities.h>

// The handshake must advertise exactly the features this base implements:
// TxHashSet fast sync and the peer list. PIBD/archival bits are not implemented
// here and must not be claimed to peers.
TEST_CASE("Capabilities::AdvertisedMatchesImplementedFeatures")
{
	REQUIRE((uint32_t)Capabilities::ADVERTISED == (uint32_t)Capabilities::FAST_SYNC_NODE);

	const Capabilities advertised(Capabilities::ADVERTISED);
	REQUIRE(advertised.HasCapability(Capabilities::TXHASHET_HIST));
	REQUIRE(advertised.HasCapability(Capabilities::PEER_LIST));
	REQUIRE(!advertised.HasCapability(Capabilities::FULL_HIST));
}
