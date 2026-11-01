#include <catch.hpp>

#include <Core/Models/BlockHeader.h>
#include <Core/Serialization/ByteBuffer.h>
#include <P2P/PIBD/PibdParams.h>
#include <P2P/PIBD/PibdSegmentService.h>
#include <PMMR/KernelMMR.h>
#include <PMMR/OutputPMMR.h>
#include <PMMR/RangeProofPMMR.h>
#include <PMMR/Models/SegmentIdentifier.h>

#include <filesystem.h>
#include <fstream>

TEST_CASE("PibdSegmentService::FullSweepPpCkpt", "[.]")
{
	const fs::path stateDir = "C:/DHW/.tmp/interop/pp-ckpt/.GrinPP/FLOONET/NODE/TXHASHSET";
	const fs::path headerPath = "C:/DHW/.tmp/g09-replay/fixture/header.bin";
	if (!fs::exists(stateDir / "output/pmmr_hash.bin") || !fs::exists(headerPath)) {
		WARN("pp-ckpt/fixture absent; skipping full PIBD sweep");
		return;
	}

	std::ifstream headerFile(headerPath, std::ios::binary);
	const std::vector<uint8_t> headerBytes(
		(std::istreambuf_iterator<char>(headerFile)), std::istreambuf_iterator<char>());
	ByteBuffer headerBuffer(headerBytes);
	const BlockHeader header = BlockHeader::Deserialize(headerBuffer);

	std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(stateDir);
	std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(stateDir);
	std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(stateDir);
	PibdSegmentService service(pOutputPMMR, pRangeProofPMMR, pKernelMMR);
	const VectorSegmentBitmap bitmap = service.GetOutputBitmap();
	const Hash bitmapRoot = service.GetBitmapMmrRoot();

	uint64_t outputFailures = 0;
	uint64_t rangeProofFailures = 0;
	uint64_t kernelFailures = 0;

	const uint64_t outputSegments = SegmentIdentifier::CountSegmentsRequired(
		header.GetOutputMMRSize(), PibdParams::OUTPUT_SEGMENT_HEIGHT);
	for (uint64_t idx = 0; idx < outputSegments; ++idx) {
		try {
			const auto segment = service.GetOutputSegment(
				SegmentIdentifier(PibdParams::OUTPUT_SEGMENT_HEIGHT, idx));
			segment.ValidateWithOtherRoot(
				header.GetOutputMMRSize(), &bitmap, header.GetOutputRoot(),
				header.GetOutputMMRSize(), bitmapRoot, false);
		} catch (const std::exception& e) {
			if (outputFailures++ < 3) {
				WARN("output segment " << idx << " failed: " << e.what());
			}
		}
	}

	const uint64_t rangeProofSegments = SegmentIdentifier::CountSegmentsRequired(
		header.GetOutputMMRSize(), PibdParams::RANGEPROOF_SEGMENT_HEIGHT);
	for (uint64_t idx = 0; idx < rangeProofSegments; ++idx) {
		try {
			const auto segment = service.GetRangeProofSegment(
				SegmentIdentifier(PibdParams::RANGEPROOF_SEGMENT_HEIGHT, idx));
			segment.ValidateWithBitmap(
				header.GetOutputMMRSize(), bitmap, header.GetRangeProofRoot());
		} catch (const std::exception& e) {
			if (rangeProofFailures++ < 3) {
				WARN("rangeproof segment " << idx << " failed: " << e.what());
			}
		}
	}

	const uint64_t kernelSegments = SegmentIdentifier::CountSegmentsRequired(
		header.GetKernelMMRSize(), PibdParams::KERNEL_SEGMENT_HEIGHT);
	for (uint64_t idx = 0; idx < kernelSegments; ++idx) {
		try {
			const auto segment = service.GetKernelSegment(
				SegmentIdentifier(PibdParams::KERNEL_SEGMENT_HEIGHT, idx));
			segment.Validate(header.GetKernelMMRSize(), header.GetKernelRoot());
		} catch (const std::exception& e) {
			if (kernelFailures++ < 3) {
				WARN("kernel segment " << idx << " failed: " << e.what());
			}
		}
	}

	WARN("sweep: output " << outputFailures << "/" << outputSegments
		<< ", rangeproof " << rangeProofFailures << "/" << rangeProofSegments
		<< ", kernel " << kernelFailures << "/" << kernelSegments);
	CHECK(outputFailures == 0);
	CHECK(rangeProofFailures == 0);
	CHECK(kernelFailures == 0);
}
