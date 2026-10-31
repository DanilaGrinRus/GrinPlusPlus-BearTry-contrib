#include <catch.hpp>

#include <BlockChain/ChainConsistency.h>

TEST_CASE("ChainConsistency::ConsistentStoresNeedNoRepair")
{
	const ChainRepairPlan plan = ComputeChainRepair(100, 100, 50);
	REQUIRE(plan.IsConsistent());
}

TEST_CASE("ChainConsistency::HeaderMmrAheadIsRewoundToChain")
{
	// Crash after committing extra MMR leaves but before the chain index.
	const ChainRepairPlan plan = ComputeChainRepair(120, 100, 50);
	REQUIRE(!plan.IsConsistent());
	REQUIRE(plan.RewindHeaderMmr);
	REQUIRE(!plan.RewindCandidate);
	REQUIRE(!plan.RewindConfirmed);
	REQUIRE(plan.TargetEntries == 100);
}

TEST_CASE("ChainConsistency::ChainAheadIsRewoundToHeaderMmr")
{
	// Crash after committing chain index entries but before the MMR.
	const ChainRepairPlan plan = ComputeChainRepair(80, 100, 50);
	REQUIRE(!plan.IsConsistent());
	REQUIRE(!plan.RewindHeaderMmr);
	REQUIRE(plan.RewindCandidate);
	// The confirmed chain at 51 entries is already below the target and must not
	// be rewound forward.
	REQUIRE(!plan.RewindConfirmed);
	REQUIRE(plan.TargetEntries == 80);
}

TEST_CASE("ChainConsistency::ConfirmedChainCannotBeAheadOfCandidate")
{
	const ChainRepairPlan plan = ComputeChainRepair(100, 100, 120);
	REQUIRE(!plan.IsConsistent());
	REQUIRE(!plan.RewindHeaderMmr);
	REQUIRE(!plan.RewindCandidate);
	REQUIRE(plan.RewindConfirmed);
	REQUIRE(plan.TargetEntries == 100);
}
