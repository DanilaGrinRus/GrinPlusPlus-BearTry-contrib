#pragma once

#include "InMemoryPmmr.h"
#include "Segment.h"
#include "SegmentBitmap.h"
#include "SegmentException.h"
#include "SegmentIdentifier.h"
#include "SegmentProof.h"

#include "../Common/MMRUtil.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

// Generate PMMR segments from an in-memory MMR.
// Ported from grin v5.5.2 Segment::from_pmmr / SegmentProof::generate:
//   * without a bitmap the segment is unpruned (all leaves in range, no hashes);
//   * with a bitmap the segment is pruned like a real PIBD peer would serve it:
//     only required leaves are included, internal hashes are always included and
//     a fully pruned segment carries the first unpruned parent hash.
template<class T>
class SegmentGenerator
{
public:
	static Segment<T> Generate(const InMemoryPmmr<T>& mmr, const SegmentIdentifier& identifier)
	{
		return Generate(mmr, identifier, nullptr);
	}

	static Segment<T> Generate(
		const InMemoryPmmr<T>& mmr,
		const SegmentIdentifier& identifier,
		const SegmentBitmap* bitmap)
	{
		const uint64_t mmrSize = mmr.GetSize();
		if (identifier.SegmentUnprunedSize(mmrSize) == 0) {
			throw SegmentException(
				ESegmentError::NonExistent,
				"Segment " + std::to_string(identifier.GetIdx()) + " does not exist at this MMR size");
		}

		const std::pair<uint64_t, uint64_t> range = identifier.SegmentPosRange(mmrSize);

		std::vector<uint64_t> hashPositions;
		std::vector<Hash> hashes;
		std::vector<uint64_t> leafPositions;
		std::vector<T> leaves;
		for (uint64_t pos0 = range.first; ; ++pos0) {
			if (MMRUtil::IsLeaf(pos0)) {
				if (bitmap == nullptr || IsSegmentLeafRequired(pos0, mmrSize, *bitmap)) {
					leafPositions.push_back(pos0);
					leaves.push_back(mmr.GetLeafData(pos0));
				}
			} else if (bitmap != nullptr) {
				hashPositions.push_back(pos0);
				hashes.push_back(mmr.GetHash(pos0));
			}

			if (pos0 == range.second) {
				break;
			}
		}

		std::optional<uint64_t> startPos;
		if (bitmap != nullptr && leaves.empty() && hashes.empty()) {
			// Fully pruned segment: only include the first unpruned parent hash.
			const std::vector<std::pair<uint64_t, uint64_t>> branch =
				MMRUtil::FamilyBranch(range.second, mmrSize);
			if (!branch.empty()) {
				hashPositions.push_back(branch[0].first);
				hashes.push_back(mmr.GetHash(branch[0].first));
				startPos = branch[0].first + 1;
			}
		}

		SegmentProof proof = GenerateProof(mmr, range.first, range.second, startPos);
		return Segment<T>(
			identifier,
			std::move(hashPositions),
			std::move(hashes),
			std::move(leafPositions),
			std::move(leaves),
			std::move(proof));
	}

private:
	static SegmentProof GenerateProof(
		const InMemoryPmmr<T>& mmr,
		const uint64_t firstPos0,
		const uint64_t lastPos0,
		const std::optional<uint64_t> startPos)
	{
		const uint64_t mmrSize = mmr.GetSize();
		std::vector<Hash> hashes;

		// 1. Siblings along the path from the subtree root to the peak.
		const std::vector<std::pair<uint64_t, uint64_t>> familyBranch =
			MMRUtil::FamilyBranch(lastPos0, mmrSize);
		for (const std::pair<uint64_t, uint64_t>& branch : familyBranch) {
			if (startPos.has_value() && branch.first < startPos.value()) {
				continue;
			}

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
