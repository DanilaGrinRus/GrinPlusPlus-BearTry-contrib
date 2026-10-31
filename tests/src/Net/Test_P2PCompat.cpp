#include <catch.hpp>

#include <GrinVersion.h>
#include <P2P/Capabilities.h>
#include <P2P/Common.h>
#include <P2P/Messages/MessageTypes.h>

// Wire-format pins for Grin v5.5.2 compatibility. These values are consensus on
// the network boundary: changing any of them silently breaks interop.
TEST_CASE("GrinVersion::MatchesRelease")
{
	// The repository is Grin++ v1.2.9; do not pretend to be the Rust reference.
	REQUIRE(GRINPP_VERSION == "1.2.9");
	REQUIRE(P2P::USER_AGENT == "Grin++ 1.2.9");
}

TEST_CASE("Capabilities::WireFormat")
{
	REQUIRE((uint32_t)Capabilities::HEADER_HIST == 0x01);
	REQUIRE((uint32_t)Capabilities::FULL_HIST == 0x01);
	REQUIRE((uint32_t)Capabilities::TXHASHET_HIST == 0x02);
	REQUIRE((uint32_t)Capabilities::PEER_LIST == 0x04);
	REQUIRE((uint32_t)Capabilities::TX_KERNEL_HASH == 0x08);
	REQUIRE((uint32_t)Capabilities::PIBD_HIST == 0x10);
	REQUIRE((uint32_t)Capabilities::BLOCK_HIST == 0x20);
	REQUIRE((uint32_t)Capabilities::PIBD_HIST_1 == 0x40);
	REQUIRE((uint32_t)Capabilities::PIHD_HIST == 0x80);

	// PIBD serving is implemented, so both PIBD bits are advertised; peers may
	// request bitmap/output/rangeproof/kernel segments from us at our flushed state.
	REQUIRE(Capabilities(Capabilities::ADVERTISED).HasCapability(Capabilities::PIBD_HIST));
	REQUIRE(Capabilities(Capabilities::ADVERTISED).HasCapability(Capabilities::PIBD_HIST_1));
	REQUIRE(!Capabilities(Capabilities::ADVERTISED).HasCapability(Capabilities::BLOCK_HIST));

	// Grin 5.5.2 has no PIHD messages (its enum ends at KernelSegment = 28), so
	// the extension bit must never be advertised; header sync stays legacy.
	REQUIRE(!Capabilities(Capabilities::ADVERTISED).HasCapability(Capabilities::PIHD_HIST));
	REQUIRE(Capabilities(Capabilities::PIBD_HIST_1 | Capabilities::PIHD_HIST)
		.HasCapability(Capabilities::PIBD_HIST_1));
	REQUIRE(Capabilities(Capabilities::PIBD_HIST_1 | Capabilities::PIHD_HIST)
		.HasCapability(Capabilities::PIHD_HIST));
}

TEST_CASE("MessageTypes::PibdIds")
{
	REQUIRE((int)MessageTypes::GetOutputBitmapSegment == 21);
	REQUIRE((int)MessageTypes::OutputBitmapSegment == 22);
	REQUIRE((int)MessageTypes::GetOutputSegment == 23);
	REQUIRE((int)MessageTypes::OutputSegment == 24);
	REQUIRE((int)MessageTypes::GetRangeProofSegment == 25);
	REQUIRE((int)MessageTypes::RangeProofSegment == 26);
	REQUIRE((int)MessageTypes::GetKernelSegment == 27);
	REQUIRE((int)MessageTypes::KernelSegment == 28);
}

TEST_CASE("MessageTypes::MaximumSizes")
{
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::Hand) == 128);
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::Shake) == 88);
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::GetHeaders) == 1 + 32 * P2P::MAX_LOCATORS);
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::Headers) == 2 + 365 * P2P::MAX_BLOCK_HEADERS);
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::TxHashSetRequest) == 40);
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::TxHashSetArchive) == 64);

	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::GetOutputSegment) == 41);
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::GetKernelSegment) == 41);
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::OutputSegment) == 2 * P2P::MAX_BLOCK_SIZE);
	REQUIRE(MessageTypes::GetMaximumSize(MessageTypes::KernelSegment) == 2 * P2P::MAX_BLOCK_SIZE);
}
