#include <catch.hpp>

#include <P2P/Capabilities.h>
#include <P2P/Messages/MessageTypes.h>

// Wire-format pins for Grin v5.5.2 compatibility. These values are consensus on
// the network boundary: changing any of them silently breaks interop.
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

	// PIBD is not implemented yet: advertising it would make Grin 5.5.2 peers
	// expect segment serving that does not exist.
	REQUIRE(!Capabilities(Capabilities::ADVERTISED).HasCapability(Capabilities::PIBD_HIST));
	REQUIRE(!Capabilities(Capabilities::ADVERTISED).HasCapability(Capabilities::PIBD_HIST_1));
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
