#include <catch.hpp>

#include <API/Node/Handlers/NodeStatus.h>

TEST_CASE("NodeStatus::ChainShortNamesMatchRust")
{
	REQUIRE(NodeStatus::ChainShortName(Environment::MAINNET) == "main");
	REQUIRE(NodeStatus::ChainShortName(Environment::FLOONET) == "floonet");
	REQUIRE(NodeStatus::ChainShortName(Environment::AUTOMATED_TESTING) == "test");
}

TEST_CASE("NodeStatus::SyncStatusNamesMatchRust")
{
	// Wire names from grin v5.5.2 api::types::SyncStatus; GET /v1/status keeps
	// the legacy C++ names in its own field and exposes these under "rust".
	REQUIRE(NodeStatus::SyncStatusName(ESyncStatus::SYNCING_HEADERS) == "header_sync");
	REQUIRE(NodeStatus::SyncStatusName(ESyncStatus::SYNCING_TXHASHSET) == "txhashset_download");
	REQUIRE(NodeStatus::SyncStatusName(ESyncStatus::PROCESSING_TXHASHSET) == "txhashset_rangeproofs_validation");
	REQUIRE(NodeStatus::SyncStatusName(ESyncStatus::TXHASHSET_SYNC_FAILED) == "txhashset_sync_failed");
	REQUIRE(NodeStatus::SyncStatusName(ESyncStatus::SYNCING_BLOCKS) == "body_sync");
	REQUIRE(NodeStatus::SyncStatusName(ESyncStatus::WAITING_FOR_PEERS) == "awaiting_peers");
	REQUIRE(NodeStatus::SyncStatusName(ESyncStatus::NOT_SYNCING) == "no_sync");
}
