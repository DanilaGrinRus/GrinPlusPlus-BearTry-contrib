#pragma once

#include "../../PMMR/Common/MMRHashUtil.h"
#include "../../PMMR/Common/MMRUtil.h"
#include "../../PMMR/Models/SegmentException.h"

#include <Core/Serialization/Serializer.h>
#include <Crypto/Models/Hash.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Rebuild the root of a possibly pruned PMMR from the leaves we hold plus the
// subtree hashes carried by segments.
//
// This is the in-memory equivalent of what the desegmenter writes into the PMMR
// hash file: every node is either derived from leaf data, computed from its
// children, or taken from a stored pruned-subtree hash. Nodes below a pruned
// subtree root have no hash at all (that is what makes the tree pruned).
template<class T>
inline Hash ComputePrunedTreeRoot(
	const uint64_t mmrSize,
	const std::vector<std::pair<uint64_t, T>>& leavesByLeafIndex,
	const std::map<uint64_t, Hash>& storedHashes)
{
	if (mmrSize == 0) {
		throw SegmentException(ESegmentError::NonExistent, "Empty PMMR has no root");
	}

	const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrSize);
	std::map<uint64_t, T> leaves;
	for (const std::pair<uint64_t, T>& leaf : leavesByLeafIndex) {
		if (leaf.first >= numLeaves) {
			throw SegmentException(
				ESegmentError::Mismatch,
				"Leaf index " + std::to_string(leaf.first) + " is out of range for this MMR");
		}

		leaves.insert_or_assign(leaf.first, leaf.second);
	}

	std::vector<std::optional<Hash>> hashes((size_t)mmrSize);
	uint64_t leafIndex = 0;
	for (uint64_t pos0 = 0; pos0 < mmrSize; ++pos0) {
		if (MMRUtil::IsLeaf(pos0)) {
			const auto leaf = leaves.find(leafIndex);
			if (leaf != leaves.end()) {
				Serializer serializer;
				leaf->second.Serialize(serializer);
				hashes[(size_t)pos0] = MMRHashUtil::HashLeafWithIndex(serializer.GetBytes(), pos0);
			}

			++leafIndex;
			continue;
		}

		const uint64_t height = MMRUtil::BintreePostorderHeight(pos0);
		const uint64_t leftChild = pos0 - (1ULL << height);
		const uint64_t rightChild = pos0 - 1;
		const std::optional<Hash>& leftHash = hashes[(size_t)leftChild];
		const std::optional<Hash>& rightHash = hashes[(size_t)rightChild];

		if (!leftHash.has_value() && !rightHash.has_value()) {
			// Both children are pruned: this node must be a stored subtree root.
			const auto stored = storedHashes.find(pos0);
			if (stored == storedHashes.end()) {
				throw SegmentException(
					ESegmentError::MissingHash,
					"Missing pruned-subtree hash at position " + std::to_string(pos0));
			}

			hashes[(size_t)pos0] = stored->second;
		} else if (!leftHash.has_value()) {
			const auto stored = storedHashes.find(leftChild);
			if (stored == storedHashes.end()) {
				throw SegmentException(
					ESegmentError::MissingHash,
					"Missing pruned-subtree hash at position " + std::to_string(leftChild));
			}

			hashes[(size_t)pos0] = MMRHashUtil::HashParentWithIndex(stored->second, rightHash.value(), pos0);
		} else if (!rightHash.has_value()) {
			const auto stored = storedHashes.find(rightChild);
			if (stored == storedHashes.end()) {
				throw SegmentException(
					ESegmentError::MissingHash,
					"Missing pruned-subtree hash at position " + std::to_string(rightChild));
			}

			hashes[(size_t)pos0] = MMRHashUtil::HashParentWithIndex(leftHash.value(), stored->second, pos0);
		} else {
			hashes[(size_t)pos0] = MMRHashUtil::HashParentWithIndex(
				leftHash.value(), rightHash.value(), pos0);
		}
	}

	// Bag the peaks, exactly like MMRHashUtil::Root.
	Hash root = ZERO_HASH;
	const std::vector<uint64_t> peaks = MMRUtil::GetPeakIndices(mmrSize);
	for (auto iter = peaks.crbegin(); iter != peaks.crend(); ++iter) {
		const uint64_t peakPos0 = *iter;
		std::optional<Hash> peakHash = hashes[(size_t)peakPos0];
		if (!peakHash.has_value()) {
			const auto stored = storedHashes.find(peakPos0);
			if (stored == storedHashes.end()) {
				throw SegmentException(
					ESegmentError::MissingHash,
					"Missing hash for peak at position " + std::to_string(peakPos0));
			}

			peakHash = stored->second;
		}

		root = (root == ZERO_HASH)
			? peakHash.value()
			: MMRHashUtil::HashParentWithIndex(peakHash.value(), root, mmrSize);
	}

	if (root == ZERO_HASH) {
		throw SegmentException(ESegmentError::NonExistent, "Tree produced no root");
	}

	return root;
}
