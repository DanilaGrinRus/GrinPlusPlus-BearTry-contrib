#pragma once

#include <cstdint>

// Startup consistency check for the three append-only chain stores:
//   * header MMR hash file (header_mmr.bin) - one leaf per header
//   * candidate chain data file (candidate.chain) - 32-byte hash per entry
//   * confirmed chain data file (confirmed.chain)
//
// They are committed independently, so a crash between the two commits leaves
// the MMR and the chain at different heights. Loading such a state used to read
// a header beyond the MMR (access violation). The repair plan makes both sides
// agree again, preferring the chain index (it is what the rest of the DB refers
// to); extra MMR leaves are safe to drop and missing ones are recovered from the
// chain index.
struct ChainRepairPlan
{
	bool RewindHeaderMmr = false;
	bool RewindCandidate = false;
	bool RewindConfirmed = false;
	uint64_t TargetEntries = 0;
	bool IsConsistent() const noexcept
	{
		return !RewindHeaderMmr && !RewindCandidate && !RewindConfirmed;
	}
};

inline ChainRepairPlan ComputeChainRepair(
	const uint64_t headerMmrLeaves,
	const uint64_t candidateEntries,
	const uint64_t confirmedEntries) noexcept
{
	// The chain index is the reference: the confirmed chain may never be ahead
	// of the candidate chain, and the header MMR may not be ahead of the chain.
	uint64_t targetEntries = candidateEntries;
	if (headerMmrLeaves < targetEntries) {
		// Chain is ahead of the MMR: bring both chain files back to the MMR.
		targetEntries = headerMmrLeaves;
	}

	ChainRepairPlan plan;
	plan.TargetEntries = targetEntries;
	plan.RewindHeaderMmr = headerMmrLeaves > targetEntries;
	plan.RewindCandidate = candidateEntries > targetEntries;
	plan.RewindConfirmed = confirmedEntries > targetEntries;
	return plan;
}