#pragma once

#include <Core/Exceptions/DeserializationException.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Serializable.h>
#include <Crypto/Models/Hash.h>

#include <cstdint>
#include <vector>

// Merkle proof of a segment.
// Wire format matches grin v5.5.2 core/src/core/pmmr/segment.rs (SegmentProof).
class SegmentProof : public Traits::ISerializable
{
public:
	// Same bound as grin: MAX_SEGMENT_READ_ITEMS.
	static constexpr uint64_t MAX_HASHES = 1000000;

	SegmentProof(std::vector<Hash> hashes)
		: m_hashes(std::move(hashes))
	{
	}

	static SegmentProof Empty()
	{
		return SegmentProof(std::vector<Hash>());
	}

	const std::vector<Hash>& GetHashes() const noexcept { return m_hashes; }
	size_t Size() const noexcept { return m_hashes.size(); }

	void Serialize(Serializer& serializer) const final
	{
		serializer.Append<uint64_t>((uint64_t)m_hashes.size());
		for (const Hash& hash : m_hashes) {
			serializer.AppendBigInteger(hash);
		}
	}

	static SegmentProof Deserialize(ByteBuffer& byteBuffer)
	{
		const uint64_t numHashes = byteBuffer.ReadU64();
		if (numHashes > MAX_HASHES) {
			throw DESERIALIZATION_EXCEPTION_F("Segment proof declares {} hashes, limit is {}", numHashes, MAX_HASHES);
		}

		if (numHashes > byteBuffer.GetRemainingSize() / (uint64_t)HASH_SIZE) {
			throw DESERIALIZATION_EXCEPTION("Segment proof declares more hashes than the payload can hold");
		}

		std::vector<Hash> hashes;
		hashes.reserve((size_t)numHashes);
		for (uint64_t i = 0; i < numHashes; ++i) {
			hashes.push_back(byteBuffer.ReadBigInteger<32>());
		}

		return SegmentProof(std::move(hashes));
	}

private:
	std::vector<Hash> m_hashes;
};
