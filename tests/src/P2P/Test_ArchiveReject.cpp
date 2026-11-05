#include <catch.hpp>

#include <P2P/Pipeline/TxHashSetPipe.h>

TEST_CASE("Unrequested txhashset archives are rejected")
{
    REQUIRE(TxHashSetPipe::IsArchiveRequested(ESyncStatus::SYNCING_TXHASHSET));
    REQUIRE_FALSE(TxHashSetPipe::IsArchiveRequested(ESyncStatus::SYNCING_BLOCKS));
    REQUIRE_FALSE(TxHashSetPipe::IsArchiveRequested(ESyncStatus::SYNCING_HEADERS));
    REQUIRE_FALSE(TxHashSetPipe::IsArchiveRequested(ESyncStatus::WAITING_FOR_PEERS));
    REQUIRE_FALSE(TxHashSetPipe::IsArchiveRequested(ESyncStatus::PROCESSING_TXHASHSET));
    REQUIRE_FALSE(TxHashSetPipe::IsArchiveRequested(ESyncStatus::NOT_SYNCING));
}
