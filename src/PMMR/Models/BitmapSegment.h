#pragma once

#include "BitmapBlock.h"
#include "SegmentIdentifier.h"
#include "SegmentProof.h"

#include <Core/Exceptions/DeserializationException.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Serializable.h>

#include <cstdint>
#include <utility>
#include <vector>

// A bitmap segment: up to 2**height bitmap chunks plus a merkle proof.
// Wire format matches grin v5.5.2 BitmapSegment:
//   identifier (u8 height, u64 idx), u16 block_count, blocks, SegmentProof.
class BitmapSegment : public Traits::ISerializable
{
public:
	// Matches the upper end of the served PIBD bitmap segment range in grin.
	static constexpr uint8_t MAX_SEGMENT_HEIGHT = 13;

	BitmapSegment(SegmentIdentifier identifier, std::vector<BitmapBlock> blocks, SegmentProof proof)
		: m_identifier(identifier),
		m_blocks(std::move(blocks)),
		m_proof(std::move(proof))
	{
	}

	const SegmentIdentifier& GetIdentifier() const noexcept { return m_identifier; }
	const std::vector<BitmapBlock>& GetBlocks() const noexcept { return m_blocks; }
	const SegmentProof& GetProof() const noexcept { return m_proof; }

	// Total number of 1024-bit chunks encoded by the blocks, validated the same
	// way as grin's BitmapSegment::validate_blocks.
	uint64_t GetNumChunks() const
	{
		if (m_blocks.empty()) {
			throw DESERIALIZATION_EXCEPTION("Bitmap segment has no blocks");
		}

		uint64_t total = 0;
		for (size_t i = 0; i < m_blocks.size(); ++i) {
			const size_t chunks = m_blocks[i].GetNumChunks();
			if (i + 1 == m_blocks.size()) {
				if (chunks == 0) {
					throw DESERIALIZATION_EXCEPTION("Last bitmap block is empty");
				}
			} else if (chunks != BitmapBlock::MAX_CHUNKS) {
				throw DESERIALIZATION_EXCEPTION("Non-final bitmap block is not full");
			}

			if (total > UINT64_MAX - (uint64_t)chunks) {
				throw DESERIALIZATION_EXCEPTION("Bitmap segment chunk count overflows");
			}

			total += (uint64_t)chunks;
		}

		return total;
	}

	void Serialize(Serializer& serializer) const final
	{
		if (m_blocks.size() > 0xFFFF) {
			throw DESERIALIZATION_EXCEPTION("Bitmap segment has too many blocks to serialize");
		}

		m_identifier.Serialize(serializer);
		serializer.Append<uint16_t>((uint16_t)m_blocks.size());
		for (const BitmapBlock& block : m_blocks) {
			block.Serialize(serializer);
		}

		m_proof.Serialize(serializer);
	}

	static BitmapSegment Deserialize(ByteBuffer& byteBuffer)
	{
		const SegmentIdentifier identifier = SegmentIdentifier::Deserialize(byteBuffer);
		if (identifier.GetHeight() > MAX_SEGMENT_HEIGHT) {
			throw DESERIALIZATION_EXCEPTION_F(
				"Bitmap segment height {} exceeds the maximum {}", identifier.GetHeight(), MAX_SEGMENT_HEIGHT);
		}

		const uint16_t numBlocks = byteBuffer.ReadU16();
		if (numBlocks == 0) {
			throw DESERIALIZATION_EXCEPTION("Bitmap segment declares zero blocks");
		}

		const uint64_t maxChunks = 1ULL << identifier.GetHeight();
		const uint64_t maxBlocks = (maxChunks + BitmapBlock::MAX_CHUNKS - 1) / BitmapBlock::MAX_CHUNKS;
		if ((uint64_t)numBlocks > maxBlocks) {
			throw DESERIALIZATION_EXCEPTION_F(
				"Bitmap segment declares {} blocks, maximum is {}", numBlocks, maxBlocks);
		}

		// Leaf offset must be representable (same checked multiply as grin).
		const uint64_t capacity = identifier.SegmentCapacity();
		if (identifier.GetIdx() != 0 && capacity > UINT64_MAX / identifier.GetIdx()) {
			throw DESERIALIZATION_EXCEPTION("Bitmap segment leaf offset overflows");
		}

		std::vector<BitmapBlock> blocks;
		blocks.reserve((size_t)numBlocks);
		for (uint16_t i = 0; i < numBlocks; ++i) {
			blocks.push_back(BitmapBlock::Deserialize(byteBuffer));
		}

		BitmapSegment segment(identifier, std::move(blocks), SegmentProof::Empty());
		if (segment.GetNumChunks() > maxChunks) {
			throw DESERIALIZATION_EXCEPTION_F(
				"Bitmap segment declares more chunks than height {} allows", identifier.GetHeight());
		}

		SegmentProof proof = SegmentProof::Deserialize(byteBuffer);
		return BitmapSegment(identifier, std::move(segment.m_blocks), std::move(proof));
	}

private:
	SegmentIdentifier m_identifier;
	std::vector<BitmapBlock> m_blocks;
	SegmentProof m_proof;
};
