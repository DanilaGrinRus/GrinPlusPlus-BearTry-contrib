#include <catch.hpp>

#include <Common/Util/BitUtil.h>
#include <Core/Models/Fee.h>
#include <Core/Models/Features.h>
#include <Core/Models/OutputIdentifier.h>
#include <Core/Models/TransactionKernel.h>
#include <Crypto/Models/Commitment.h>
#include <Crypto/Models/Hash.h>
#include <Crypto/Models/RangeProof.h>
#include <Crypto/Models/Signature.h>
#include <P2P/PIBD/PibdParams.h>
#include <P2P/PIBD/PibdSegmentService.h>
#include <P2P/PIBD/PibdSession.h>
#include <P2P/PIBD/PibdSnapshot.h>
#include <P2P/PIBD/PibdStateWriter.h>
#include <PMMR/Common/MMRUtil.h>
#include <PMMR/KernelMMR.h>
#include <PMMR/Models/BitmapChunk.h>
#include <PMMR/Models/BitmapSegment.h>
#include <PMMR/Models/InMemoryPmmr.h>
#include <PMMR/Models/SegmentGenerator.h>
#include <PMMR/Models/SegmentIdentifier.h>
#include <PMMR/OutputPMMR.h>
#include <PMMR/RangeProofPMMR.h>

#include <filesystem.h>

#include <cstdint>
#include <vector>

namespace
{
	Commitment MakeCommitment(const uint8_t seed)
	{
		std::vector<uint8_t> bytes(33, seed);
		return Commitment(CBigInteger<33>(std::move(bytes)));
	}

	Signature MakeSignature(const uint8_t seed)
	{
		std::vector<uint8_t> bytes(64, seed);
		return Signature(CBigInteger<64>(std::move(bytes)));
	}

	OutputIdentifier MakeOutput(const uint8_t seed)
	{
		return OutputIdentifier(EOutputFeatures::DEFAULT, MakeCommitment(seed));
	}

	RangeProof MakeRangeProof(const uint8_t seed)
	{
		return RangeProof(std::vector<unsigned char>(675, seed));
	}

	TransactionKernel MakeKernel(const uint8_t seed)
	{
		return TransactionKernel(
			EKernelFeatures::DEFAULT_KERNEL, Fee(0, 0), 0, MakeCommitment(seed), MakeSignature(seed));
	}

	uint64_t MmrSizeForLeaves(const uint64_t numLeaves)
	{
		return 2 * numLeaves - (uint64_t)BitUtil::CountBitsSet(numLeaves);
	}

	template<class T, class TFactory>
	InMemoryPmmr<T> MakeTree(const uint64_t numLeaves, TFactory factory)
	{
		std::vector<T> leaves;
		leaves.reserve((size_t)numLeaves);
		for (uint64_t i = 0; i < numLeaves; ++i) {
			leaves.push_back(factory((uint8_t)(i + 1)));
		}

		return InMemoryPmmr<T>(std::move(leaves));
	}

	// Builds a complete synthetic horizon (64 outputs, bitmap {0}, 2 kernels) and
	// writes it to disk exactly like a desegmenter would.
	PibdSnapshot WriteTestState(const fs::path& stateDir)
	{
		const uint64_t numOutputs = 64;

		// All outputs unspent: the served bitmap derived from the leaf set equals
		// the horizon bitmap. Pruned serving waits on the rebuilder fix recorded as
		// G-26 (interior nodes of a pruned subtree must not require stored hashes).
		VectorSegmentBitmap bitmap(numOutputs);
		for (uint64_t leafIndex = 0; leafIndex < numOutputs; ++leafIndex) {
			bitmap.Set(leafIndex, true);
		}

		std::vector<BitmapChunk> chunks(1);
		for (size_t bit = 0; bit < numOutputs; ++bit) {
			chunks[0].Set(bit, true);
		}
		const InMemoryPmmr<BitmapChunk> bitmapMmr(std::move(chunks));
		const InMemoryPmmr<OutputIdentifier> outputs = MakeTree<OutputIdentifier>(numOutputs, MakeOutput);
		const InMemoryPmmr<RangeProof> rangeProofs = MakeTree<RangeProof>(numOutputs, MakeRangeProof);
		const InMemoryPmmr<TransactionKernel> kernels = MakeTree<TransactionKernel>(2, MakeKernel);

		PibdDownload::Horizon horizon;
		horizon.BlockHash = Hash::ValueOf(31);
		horizon.Height = 999;
		horizon.OutputMmrSize = outputs.GetSize();
		horizon.KernelMmrSize = kernels.GetSize();
		horizon.OutputRoot = outputs.GetRoot();
		horizon.RangeProofRoot = rangeProofs.GetRoot();
		horizon.KernelRoot = kernels.GetRoot();
		horizon.BitmapRoot = bitmapMmr.GetRoot();

		PibdSession session(horizon);
		REQUIRE(session.AcceptResponse(SegmentResponseMessage::CreateOutputBitmap(
			horizon.BlockHash,
			BitmapSegment::FromSegment(SegmentGenerator<BitmapChunk>::Generate(
				bitmapMmr, SegmentIdentifier(PibdParams::BITMAP_SEGMENT_HEIGHT, 0))),
			horizon.OutputRoot)));
		REQUIRE(session.AcceptResponse(SegmentResponseMessage::CreateOutputs(
			horizon.BlockHash,
			SegmentGenerator<OutputIdentifier>::Generate(
				outputs, SegmentIdentifier(PibdParams::OUTPUT_SEGMENT_HEIGHT, 0), &bitmap),
			horizon.OutputRoot)));
		REQUIRE(session.AcceptResponse(SegmentResponseMessage::CreateRangeProofs(
			horizon.BlockHash,
			SegmentGenerator<RangeProof>::Generate(
				rangeProofs, SegmentIdentifier(PibdParams::RANGEPROOF_SEGMENT_HEIGHT, 0), &bitmap))));
		REQUIRE(session.AcceptResponse(SegmentResponseMessage::CreateKernels(
			horizon.BlockHash,
			SegmentGenerator<TransactionKernel>::Generate(
				kernels, SegmentIdentifier(PibdParams::KERNEL_SEGMENT_HEIGHT, 0)))));

		const PibdSnapshot snapshot = BuildSnapshot(session.GetDownload());
		PibdStateWriter::Write(snapshot, stateDir);
		return snapshot;
	}
}

TEST_CASE("PibdSegmentService::ServesStateThatValidates")
{
	const fs::path stateDir = fs::temp_directory_path() / "pibd-serving-test";
	std::error_code error;
	fs::remove_all(stateDir, error);

	const PibdSnapshot written = WriteTestState(stateDir);

	// Serve from the files a node would read back.
	std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(stateDir);
	std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(stateDir);
	std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(stateDir);

	PibdSegmentService service(pOutputPMMR, pRangeProofPMMR, pKernelMMR);
	REQUIRE(service.GetOutputMmrSize() == written.OutputMmrSize);
	REQUIRE(service.GetKernelMmrSize() == written.KernelMmrSize);

	// The served bitmap is the horizon bitmap taken from the leaf set.
	const VectorSegmentBitmap servedBitmap = service.GetOutputBitmap();
	for (uint64_t leafIndex = 0; leafIndex < 64; ++leafIndex) {
		REQUIRE(servedBitmap.Contains(leafIndex));
	}

	// A fresh downloader receives the served segments and rebuilds the same state.
	PibdDownload::Horizon horizon = [&written] {
		PibdDownload::Horizon h;
		h.BlockHash = written.BlockHash;
		h.Height = written.Height;
		h.OutputMmrSize = written.OutputMmrSize;
		h.KernelMmrSize = written.KernelMmrSize;
		h.OutputRoot = written.OutputRoot;
		h.RangeProofRoot = written.RangeProofRoot;
		h.KernelRoot = written.KernelRoot;
		h.BitmapRoot = written.BitmapRoot;
		return h;
	}();

	PibdSession downloader(horizon);
	REQUIRE(downloader.AcceptResponse(SegmentResponseMessage::CreateOutputBitmap(
		horizon.BlockHash,
		service.GetBitmapSegment(SegmentIdentifier(PibdParams::BITMAP_SEGMENT_HEIGHT, 0)),
		horizon.OutputRoot)));
	REQUIRE(downloader.AcceptResponse(SegmentResponseMessage::CreateOutputs(
		horizon.BlockHash,
		service.GetOutputSegment(SegmentIdentifier(PibdParams::OUTPUT_SEGMENT_HEIGHT, 0)),
		horizon.OutputRoot)));
	REQUIRE(downloader.AcceptResponse(SegmentResponseMessage::CreateRangeProofs(
		horizon.BlockHash,
		service.GetRangeProofSegment(SegmentIdentifier(PibdParams::RANGEPROOF_SEGMENT_HEIGHT, 0)))));
	REQUIRE(downloader.AcceptResponse(SegmentResponseMessage::CreateKernels(
		horizon.BlockHash,
		service.GetKernelSegment(SegmentIdentifier(PibdParams::KERNEL_SEGMENT_HEIGHT, 0)))));

	REQUIRE(downloader.GetDownload().IsComplete());

	const PibdSnapshot rebuilt = BuildSnapshot(downloader.GetDownload());
	REQUIRE(rebuilt.Outputs.size() == written.Outputs.size());
	REQUIRE(rebuilt.RangeProofs.size() == written.RangeProofs.size());
	REQUIRE(rebuilt.Kernels.size() == written.Kernels.size());

	for (size_t i = 0; i < rebuilt.Outputs.size(); ++i) {
		REQUIRE(rebuilt.Outputs[i].first == written.Outputs[i].first);
	}

	fs::remove_all(stateDir, error);
}
