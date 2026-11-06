#include <catch.hpp>

#include <BlockChain/ChainConsistency.h>

namespace
{
	bool Contains(const std::string& text, const std::string& expected)
	{
		return text.find(expected) != std::string::npos;
	}
}

TEST_CASE("ChainConsistency::ConsistentStoresPass")
{
	const ChainStoreCheck check = CheckChainStores(7, 4, 4, 1, true);
	REQUIRE(check.Consistent);
	REQUIRE_FALSE(check.Fresh);
	REQUIRE_FALSE(check.MmrAhead);
	REQUIRE_FALSE(check.MmrBehind);
	REQUIRE_FALSE(check.ConfirmedAhead);
	REQUIRE(check.Error.empty());
}

TEST_CASE("ChainConsistency::FreshStoreIsAllowed")
{
	// Candidate contains genesis only, the MMR is empty and the block DB does
	// not have genesis yet; ChainState::Create initialises the MMR.
	const ChainStoreCheck check = CheckChainStores(0, 0, 1, 1, false);
	REQUIRE(check.Consistent);
	REQUIRE(check.Fresh);
	REQUIRE(check.Error.empty());
}

TEST_CASE("ChainConsistency::RejectsGenesisInDatabaseWithEmptyMmr")
{
	// A lost/truncated MMR must not be silently re-initialised by appending
	// genesis again; this state is refused instead.
	const ChainStoreCheck check = CheckChainStores(0, 0, 1, 1, true);
	REQUIRE_FALSE(check.Consistent);
	REQUIRE(check.MmrBehind);
	REQUIRE(Contains(check.Error, "contains genesis but the header MMR is empty"));
}

TEST_CASE("ChainConsistency::RejectsImpossibleMmrNodeCount")
{
	const ChainStoreCheck check = CheckChainStores(5, 0, 4, 1, true);
	REQUIRE_FALSE(check.Consistent);
	REQUIRE(Contains(check.Error, "not a valid MMR size"));
}

TEST_CASE("ChainConsistency::RejectsLeafCountNotMatchingNodes")
{
	const ChainStoreCheck check = CheckChainStores(7, 3, 3, 1, true);
	REQUIRE_FALSE(check.Consistent);
	REQUIRE(Contains(check.Error, "does not match its node count"));
}

TEST_CASE("ChainConsistency::DetectsMmrAheadOfChainWithoutRepair")
{
	const ChainStoreCheck check = CheckChainStores(8, 5, 4, 1, true);
	REQUIRE_FALSE(check.Consistent);
	REQUIRE(check.MmrAhead);
	REQUIRE(Contains(check.Error, "Header MMR is ahead of the candidate chain"));
}

TEST_CASE("ChainConsistency::DetectsMmrBehindChainWithoutRepair")
{
	const ChainStoreCheck check = CheckChainStores(4, 3, 4, 1, true);
	REQUIRE_FALSE(check.Consistent);
	REQUIRE(check.MmrBehind);
	REQUIRE(Contains(check.Error, "Candidate chain is ahead of the header MMR"));
}

TEST_CASE("ChainConsistency::DetectsConfirmedAheadOfCandidate")
{
	const ChainStoreCheck check = CheckChainStores(7, 4, 4, 5, true);
	REQUIRE_FALSE(check.Consistent);
	REQUIRE(check.ConfirmedAhead);
	REQUIRE(Contains(check.Error, "Confirmed chain is ahead of the candidate chain"));
}

TEST_CASE("ChainConsistency::ConfirmedAheadWinsOverFreshStore")
{
	const ChainStoreCheck check = CheckChainStores(0, 0, 1, 2, false);
	REQUIRE_FALSE(check.Consistent);
	REQUIRE(check.ConfirmedAhead);
}

TEST_CASE("ChainConsistency::RejectsEmptyChainIndexes")
{
	const ChainStoreCheck candidate = CheckChainStores(0, 0, 0, 0, false);
	REQUIRE_FALSE(candidate.Consistent);
	REQUIRE(Contains(candidate.Error, "Candidate chain is empty"));

	const ChainStoreCheck confirmed = CheckChainStores(7, 4, 4, 0, true);
	REQUIRE_FALSE(confirmed.Consistent);
	REQUIRE(Contains(confirmed.Error, "Confirmed chain is empty"));
}
