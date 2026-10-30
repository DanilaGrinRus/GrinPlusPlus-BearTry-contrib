#pragma once

#include "PibdDownload.h"

#include "../../PMMR/Common/MMRUtil.h"
#include "../../PMMR/Models/InMemoryPmmr.h"
#include "PibdTreeRebuilder.h"
#include "../../PMMR/Models/SegmentException.h"

#include <Core/Models/OutputIdentifier.h>
#include <Core/Models/TransactionKernel.h>
#include <Crypto/Models/Hash.h>
#include <Crypto/Models/RangeProof.h>

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

// Result of a completed PIBD download, in the form the chain needs to apply it:
// every unpruned leaf tagged with its 0-based leaf index, plus the output bitmap
// that says which outputs are still unspent.
//
// Building the snapshot re-verifies the kernel tree end to end (kernels are never
// pruned, so the whole tree can be rebuilt from the received leaves) and checks
// that outputs and rangeproofs were pruned identically.
struct PibdSnapshot
{
	uint64_t Height = 0;
	Hash BlockHash;
	uint64_t OutputMmrSize = 0;
	uint64_t KernelMmrSize = 0;

	// (leaf index, leaf) sorted by leaf index; only the unpruned leaves.
	std::vector<std::pair<uint64_t, OutputIdentifier>> Outputs;
	std::vector<std::pair<uint64_t, RangeProof>> RangeProofs;
	std::vector<std::pair<uint64_t, TransactionKernel>> Kernels;

	// Pruned-subtree hashes carried by the segments (MMR position -> hash).
	std::map<uint64_t, Hash> OutputHashes;
	std::map<uint64_t, Hash> RangeProofHashes;
	std::map<uint64_t, Hash> KernelHashes;

	VectorSegmentBitmap Bitmap;
};

namespace PibdSnapshotDetail
{
	template<class T>
	std::vector<std::pair<uint64_t, T>> ToLeafIndices(std::vector<std::pair<uint64_t, T>> leaves)
	{
		for (std::pair<uint64_t, T>& leaf : leaves) {
			const std::optional<uint64_t> leafIndex = MMRUtil::PmmrLeafToInsertionIndex(leaf.first);
			if (!leafIndex.has_value()) {
				throw SegmentException(
					ESegmentError::Mismatch,
					"Leaf position " + std::to_string(leaf.first) + " is not a leaf");
			}

			leaf.first = leafIndex.value();
		}

		std::sort(
			leaves.begin(),
			leaves.end(),
			[](const std::pair<uint64_t, T>& lhs, const std::pair<uint64_t, T>& rhs) {
				return lhs.first < rhs.first;
			});

		for (size_t i = 1; i < leaves.size(); ++i) {
			if (leaves[i].first == leaves[i - 1].first) {
				throw SegmentException(
					ESegmentError::Mismatch,
					"Duplicate leaf index " + std::to_string(leaves[i].first));
			}
		}

		return leaves;
	}
}

// Convert a completed download into a snapshot, re-verifying the parts that can
// be verified from the leaf data alone.
inline PibdSnapshot BuildSnapshot(const PibdDownload& download)
{
	if (!download.IsComplete()) {
		throw SegmentException(ESegmentError::NonExistent, "PIBD download is not complete");
	}

	PibdSnapshot snapshot;
	snapshot.Height = download.GetHorizon().Height;
	snapshot.BlockHash = download.GetHorizon().BlockHash;
	snapshot.OutputMmrSize = download.GetHorizon().OutputMmrSize;
	snapshot.KernelMmrSize = download.GetHorizon().KernelMmrSize;
	snapshot.Bitmap = download.GetOutputBitmap();
	snapshot.Outputs = PibdSnapshotDetail::ToLeafIndices(download.GetOutputs().GetLeaves());
	snapshot.RangeProofs = PibdSnapshotDetail::ToLeafIndices(download.GetRangeProofs().GetLeaves());
	snapshot.Kernels = PibdSnapshotDetail::ToLeafIndices(download.GetKernels().GetLeaves());
	snapshot.OutputHashes = download.GetOutputs().GetStoredHashes();
	snapshot.RangeProofHashes = download.GetRangeProofs().GetStoredHashes();
	snapshot.KernelHashes = download.GetKernels().GetStoredHashes();

	// Kernels are never pruned: the whole tree must rebuild to the horizon root.
	const uint64_t kernelLeaves = MMRUtil::GetNumLeaves(snapshot.KernelMmrSize);
	if ((uint64_t)snapshot.Kernels.size() != kernelLeaves) {
		throw SegmentException(
			ESegmentError::Mismatch,
			"Received " + std::to_string(snapshot.Kernels.size()) + " kernels, expected " +
				std::to_string(kernelLeaves));
	}

	// Rebuild every tree from the leaves and the pruned-subtree hashes and check
	// it against the horizon. This is the same computation the desegmenter will
	// use to write the PMMR hash files.
	if (ComputePrunedTreeRoot(snapshot.KernelMmrSize, snapshot.Kernels, snapshot.KernelHashes)
		!= download.GetHorizon().KernelRoot) {
		throw SegmentException(
			ESegmentError::Mismatch, "Rebuilt kernel tree root does not match the horizon");
	}

	if (ComputePrunedTreeRoot(snapshot.OutputMmrSize, snapshot.Outputs, snapshot.OutputHashes)
		!= download.GetHorizon().OutputRoot) {
		throw SegmentException(
			ESegmentError::Mismatch, "Rebuilt output tree root does not match the horizon");
	}

	if (ComputePrunedTreeRoot(snapshot.OutputMmrSize, snapshot.RangeProofs, snapshot.RangeProofHashes)
		!= download.GetHorizon().RangeProofRoot) {
		throw SegmentException(
			ESegmentError::Mismatch, "Rebuilt rangeproof tree root does not match the horizon");
	}

	// Outputs and rangeproofs are pruned with the same bitmap, so their leaf
	// index sets must be identical.
	if (snapshot.Outputs.size() != snapshot.RangeProofs.size()) {
		throw SegmentException(
			ESegmentError::Mismatch, "Output and rangeproof leaf counts differ");
	}

	for (size_t i = 0; i < snapshot.Outputs.size(); ++i) {
		if (snapshot.Outputs[i].first != snapshot.RangeProofs[i].first) {
			throw SegmentException(
				ESegmentError::Mismatch,
				"Output and rangeproof leaf indices differ at " + std::to_string(snapshot.Outputs[i].first));
		}
	}

	return snapshot;
}
