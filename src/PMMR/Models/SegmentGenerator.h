#pragma once

#include "InMemoryPmmr.h"
#include "Segment.h"
#include "SegmentException.h"
#include "SegmentIdentifier.h"
#include "SegmentProof.h"

#include "../Common/MMRUtil.h"

#include <cstdint>
#include <utility>
#include <vector>

// Generate a PMMR segment (leaves + merkle proof) from an in-memory MMR.
// Ported from grin v5.5.2 Segment::from_pmmr / SegmentProof::generate for the
// non-prunable case, which is what serving and the round-trip tests need.
template<class T>
class SegmentGenerator
{
public:
	static Segment<T> Generate(const InMemoryPmmr<T>& mmr, const SegmentIdentifier& identifier)
	{
		const uint64_t mmrSize = mmr.GetSize();
		if (identifier.SegmentUnprunedSize(mmrSize) == 0) {
			throw SegmentException(
				ESegmentError::NonExistent,
				"Segment " + std::to_string(identifier.GetIdx()) + " does not exist at this MMR size");
		}

		const std::pair<uint64_t, uint64_t> range = identifier.SegmentPosRange(mmrSize);

		std::vector<uint64_t> leafPositions;
		std::vector<T> leaves;
		for (uint64_t pos0 = range.first; ; ++pos0) {
			if (MMRUtil::IsLeaf(pos0)) {
				leafPositions.push_back(pos0);
				leaves.push_back(mmr.GetLeafData(pos0));
			}

			if (pos0 == range.second) {
				break;
			}
		}

		SegmentProof proof = GenerateProof(mmr, range.first, range.second);
		return Segment<T>(
			identifier, {}, {}, std::move(leafPositions), std::move(leaves), std::move(proof));
	}

private:
	static SegmentProof GenerateProof(const InMemoryPmmr<T>& mmr, const uint64_t firstPos0, const uint64_t lastPos0)
	{
		const uint64_t mmrSize = mmr.GetSize();
		std::vector<Hash> hashes;

		// 1. Siblings along the path from the subtree root to the peak.
		const std::vector<std::pair<uint64_t, uint64_t>> familyBranch =
			MMRUtil::FamilyBranch(lastPos0, mmrSize);
		for (const std::pair<uint64_t, uint64_t>& branch : familyBranch) {
			hashes.push_back(mmr.GetHash(branch.second));
		}

		// 2. Bagged peaks strictly to the right of the peak.
		const uint64_t peakPos0 = familyBranch.empty() ? lastPos0 : familyBranch.back().first;
		const std::optional<Hash> rhs = mmr.BagTheRhs(peakPos0);
		if (rhs.has_value()) {
			hashes.push_back(rhs.value());
		}

		// 3. Peaks strictly to the left, right to left.
		const std::vector<uint64_t> peaks = MMRUtil::GetPeakIndices(mmrSize);
		for (auto iter = peaks.crbegin(); iter != peaks.crend(); ++iter) {
			if (*iter < firstPos0) {
				hashes.push_back(mmr.GetHash(*iter));
			}
		}

		return SegmentProof(std::move(hashes));
	}
};
