#include <catch.hpp>

#include <Core/Config/P2PConfig.h>
#include <Core/Enums/Environment.h>
#include <P2P/Capabilities.h>

TEST_CASE("P2PConfig::ReadsConfiguredP2PPort")
{
	Json::Value json;
	json[ConfigProps::P2P::P2P] = Json::Value();
	json[ConfigProps::P2P::P2P][ConfigProps::P2P::P2P_PORT] = 44556;

	const P2PConfig config(Environment::FLOONET, json);
	REQUIRE(config.GetP2PPort() == 44556);
}

TEST_CASE("P2PConfig::UsesNetworkDefaultPortWithoutOverride")
{
	const Json::Value json;
	const P2PConfig mainnet(Environment::MAINNET, json);
	const P2PConfig floonet(Environment::FLOONET, json);

	REQUIRE(mainnet.GetP2PPort() == 3414);
	REQUIRE(floonet.GetP2PPort() == 13414);
}

TEST_CASE("Capabilities::AdvertisedMatchesImplementedFeatures")
{
	// The handshake advertises the real set for this base: TxHashSet fast sync
	// and the peer list. PIBD/archival bits are not implemented here.
	REQUIRE((uint32_t)Capabilities::ADVERTISED == (uint32_t)Capabilities::FAST_SYNC_NODE);

	const Capabilities advertised(Capabilities::ADVERTISED);
	REQUIRE(advertised.HasCapability(Capabilities::TXHASHET_HIST));
	REQUIRE(advertised.HasCapability(Capabilities::PEER_LIST));
	REQUIRE(!advertised.HasCapability(Capabilities::FULL_HIST));
}
