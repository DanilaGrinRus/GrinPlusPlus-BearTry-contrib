#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <PMMR/Common/MmrStructure.h>

#include <cstdint>
#include <string>

// Startup validation for the three append-only chain stores:
//   * header MMR hash file (header_mmr.bin) - one leaf per header
//   * candidate chain data file (candidate.chain) - 32-byte hash per entry
//   * confirmed chain data file (confirmed.chain)
//
// They are committed independently, so a crash between the commits can leave
// them at different heights. Loading such a state used to read a header beyond
// the MMR (access violation). This check is intentionally non-destructive: on
// any inconsistency the result is "not consistent" and the caller must refuse
// to start rather than truncate data. The original files are left untouched so
// they can be inspected or restored.
struct ChainStoreCheck
{
	bool Consistent = false;
	// A brand-new store: the candidate chain contains only genesis and the
	// header MMR has not been initialised yet. ChainState::Create adds genesis.
	bool Fresh = false;
	bool MmrAhead = false;
	bool MmrBehind = false;
	bool ConfirmedAhead = false;
	std::string Error;
};

// Pure function: no files are touched. Inputs are the raw MMR node count, the
// MMR leaf count, the two chain-index entry counts and whether the block DB
// already contains genesis.
inline ChainStoreCheck CheckChainStores(
	const uint64_t headerMmrNodes,
	const uint64_t headerMmrLeaves,
	const uint64_t candidateEntries,
	const uint64_t confirmedEntries,
	const bool genesisInDatabase)
{
	ChainStoreCheck check;

	if (!MmrStructure::IsValidNodeCount(headerMmrNodes)) {
		check.Error =
			"Header MMR file has " + std::to_string(headerMmrNodes) +
			" nodes, which is not a valid MMR size (expected 2L - popcount(L)); "
			"refusing to start without modifying the chain stores.";
		return check;
	}

	const uint64_t expectedLeaves = MmrStructure::LeavesForNodes(headerMmrNodes);
	if (expectedLeaves != headerMmrLeaves) {
		check.Error =
			"Header MMR leaf count " + std::to_string(headerMmrLeaves) +
			" does not match its node count " + std::to_string(headerMmrNodes) +
			" (expected " + std::to_string(expectedLeaves) +
			" leaves); refusing to start without modifying the chain stores.";
		return check;
	}

	if (candidateEntries == 0) {
		check.Error =
			"Candidate chain is empty; refusing to start without modifying the chain stores.";
		return check;
	}

	if (confirmedEntries == 0) {
		check.Error =
			"Confirmed chain is empty; refusing to start without modifying the chain stores.";
		return check;
	}

	if (confirmedEntries > candidateEntries) {
		check.ConfirmedAhead = true;
		check.Error =
			"Confirmed chain is ahead of the candidate chain (" +
			std::to_string(confirmedEntries) + " > " +
			std::to_string(candidateEntries) +
			"); refusing to start without modifying the chain stores.";
		return check;
	}

	// Candidate contains only genesis and the header MMR is still empty. This is
	// a valid fresh database (ChainState::Create adds genesis) only when the
	// block DB does not already have genesis. If it does, the MMR was lost or
	// truncated and must not be silently re-initialised.
	if (candidateEntries == 1 && headerMmrLeaves == 0) {
		if (genesisInDatabase) {
			check.MmrBehind = true;
			check.Error =
				"Candidate chain contains genesis but the header MMR is empty; "
				"refusing to start without modifying the chain stores.";
			return check;
		}

		check.Consistent = true;
		check.Fresh = true;
		return check;
	}

	if (headerMmrLeaves > candidateEntries) {
		check.MmrAhead = true;
		check.Error =
			"Header MMR is ahead of the candidate chain (" +
			std::to_string(headerMmrLeaves) + " leaves > " +
			std::to_string(candidateEntries) + " entries); "
			"refusing to start without modifying the chain stores.";
		return check;
	}

	if (headerMmrLeaves < candidateEntries) {
		check.MmrBehind = true;
		check.Error =
			"Candidate chain is ahead of the header MMR (" +
			std::to_string(candidateEntries) + " entries > " +
			std::to_string(headerMmrLeaves) + " leaves); "
			"refusing to start without modifying the chain stores.";
		return check;
	}

	check.Consistent = true;
	return check;
}
