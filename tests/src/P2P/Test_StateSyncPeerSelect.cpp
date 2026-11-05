#include <catch.hpp>

#include <Net/IPAddress.h>
#include <P2P/Peer.h>
#include <P2P/Sync/StateSyncer.h>

namespace {
PeerPtr MakePeer(const std::array<uint8_t, 4>& bytes, const bool banned = false)
{
    auto peer = std::make_shared<Peer>(IPAddress::CreateV4(bytes));
    if (banned) {
        peer->Ban(EBanReason::BadTxHashSet);
    }
    return peer;
}
}

TEST_CASE("State sync selects a different suitable peer")
{
    const auto previous = MakePeer({ 127, 0, 0, 1 });
    const auto banned = MakePeer({ 127, 0, 0, 2 }, true);
    const auto other = MakePeer({ 127, 0, 0, 3 });

    const std::vector<PeerPtr> peers{ previous, banned, other };
    REQUIRE(StateSyncer::SelectAlternativePeer(peers, previous->GetIPAddress(), true) == other);
}

TEST_CASE("State sync reports no alternative when only the previous peer exists")
{
    const auto previous = MakePeer({ 127, 0, 0, 1 });
    const std::vector<PeerPtr> peers{ previous };
    REQUIRE(StateSyncer::SelectAlternativePeer(peers, previous->GetIPAddress(), true) == nullptr);
    REQUIRE(StateSyncer::SelectAlternativePeer({}, IPAddress(), false) == nullptr);
}

TEST_CASE("State sync retry succeeds when an alternative appears later")
{
    const auto previous = MakePeer({ 127, 0, 0, 1 });
    const std::vector<PeerPtr> none{ previous };
    REQUIRE(StateSyncer::SelectAlternativePeer(none, previous->GetIPAddress(), true) == nullptr);

    const auto later = MakePeer({ 127, 0, 0, 4 });
    const std::vector<PeerPtr> withAlternative{ previous, later };
    REQUIRE(StateSyncer::SelectAlternativePeer(withAlternative, previous->GetIPAddress(), true) == later);
}

TEST_CASE("ChooseAlternativePeer remembers the previous peer across retries")
{
    std::weak_ptr<ConnectionManager> connectionManager;
    StateSyncer syncer(connectionManager, nullptr);

    const auto slow = MakePeer({ 127, 0, 0, 1 });
    const auto alternative = MakePeer({ 127, 0, 0, 2 });
    const std::vector<PeerPtr> slowOnly{ slow };

    PeerPtr current = slow;
    REQUIRE(syncer.ChooseAlternativePeer(current, slowOnly) == nullptr);
    REQUIRE(current == nullptr);

    // Second request without a current peer must still exclude the slow peer.
    REQUIRE(syncer.ChooseAlternativePeer(current, slowOnly) == nullptr);

    const std::vector<PeerPtr> withAlternative{ slow, alternative };
    REQUIRE(syncer.ChooseAlternativePeer(current, withAlternative) == alternative);
}

TEST_CASE("State sync full cycle resets failed status and retries")
{
    std::vector<PeerPtr> peers;
    PeerConstPtr sentTo = nullptr;
    bool sent = false;

    StateSyncer::Dependencies dependencies;
    dependencies.getMostWorkPeers = [&peers]() { return peers; };
    dependencies.sendMessageToPeer = [&sent, &sentTo](const IMessage&, PeerConstPtr peer) {
        sent = true;
        sentTo = peer;
        return true;
    };
    dependencies.isConnected = [](const IPAddress&) { return true; };
    dependencies.getHeaderHash = [](const uint64_t) { return HASH::ZERO; };

    StateSyncer syncer(dependencies);
    const auto slow = MakePeer({ 127, 0, 0, 1 });
    syncer.SetPeerForTest(slow);
    syncer.SetLastRequestForTest(
        1,
        std::chrono::system_clock::now() - std::chrono::minutes(121)
    );
    peers.push_back(slow);

    SyncStatus status;
    status.UpdateStatus(ESyncStatus::SYNCING_TXHASHSET);
    status.UpdateHeaderStatus(
        Consensus::CUT_THROUGH_HORIZON + Consensus::STATE_SYNC_THRESHOLD + 1000,
        1
    );
    status.UpdateBlockStatus(0, 0, 0);
    status.UpdateDownloaded(0);

    REQUIRE(syncer.SyncState(status));
    REQUIRE(status.GetStatus() == ESyncStatus::TXHASHSET_SYNC_FAILED);
    REQUIRE_FALSE(sent);

    const auto alternative = MakePeer({ 127, 0, 0, 2 });
    peers.push_back(alternative);

    REQUIRE(syncer.SyncState(status));
    REQUIRE(status.GetStatus() == ESyncStatus::SYNCING_TXHASHSET);
    REQUIRE(sent);
    REQUIRE(sentTo == alternative);
}
