#include <catch.hpp>

#include <Net/IPAddress.h>
#include <P2P/Peer.h>
#include <P2P/Sync/StateSyncer.h>

TEST_CASE("State sync rotates a peer without banning it")
{
    const IPAddress ip = IPAddress::CreateV4({ 127, 0, 0, 1 });
    const auto peer = std::make_shared<Peer>(ip);
    PeerPtr syncPeer = peer;

    StateSyncer::RotatePeer(syncPeer);

    REQUIRE(syncPeer == nullptr);
    REQUIRE_FALSE(peer->IsBanned());
    REQUIRE(peer->GetBanReason() == EBanReason::None);
}
