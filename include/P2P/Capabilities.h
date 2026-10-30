#pragma once

#include <cstdint>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <json/json.h>

class Capabilities
{
public:
	enum ECapability
	{
		// We don't know (yet) what the peer can do.
		UNKNOWN = 0x00,

		// Can provide full history of headers back to genesis (for at least one
		// arbitrary fork). Grin 5.5.2 calls bit 0x01 HEADER_HIST; older Grin and
		// Grin++ called it FULL_HIST. Wire format is identical.
		HEADER_HIST = 0x01,
		FULL_HIST = HEADER_HIST,

		// Can provide the TxHashSet for some recent-enough height.
		TXHASHET_HIST = 0x02, // LIGHT_CLIENT: We can prune old range-proofs and drop unnecessary rangeproof hashes if this isn't set

		// Can provide a list of healthy peers
		PEER_LIST = 0x04,

		// Can broadcast and request transactions by kernel hash (Grin 5.5.2).
		TX_KERNEL_HASH = 0x08,

		// Can provide PIBD segments during initial byte download (Grin 5.5.2).
		PIBD_HIST = 0x10,

		// Can provide historical blocks for archival sync (Grin 5.5.2).
		BLOCK_HIST = 0x20,

		// As above, with the PIBD serialization fix (Grin PR #3705) applied.
		PIBD_HIST_1 = 0x40,

		FAST_SYNC_NODE = (TXHASHET_HIST | PEER_LIST),
		ARCHIVE_NODE = (HEADER_HIST | TXHASHET_HIST | PEER_LIST),

		// Capabilities this node advertises today. PIBD serving is implemented
		// (bitmap, output, rangeproof and kernel segments), so both PIBD bits are
		// advertised; BLOCK_HIST stays off until archival serving exists.
		ADVERTISED = (FAST_SYNC_NODE | PIBD_HIST | PIBD_HIST_1),
	};

	Capabilities(const uint32_t value)
		: m_value(value)
	{

	}

	Capabilities(const ECapability value)
		: m_value(value)
	{

	}

	inline bool IsUnknown() const noexcept { return m_value == UNKNOWN; }
	inline bool HasCapability(const ECapability capability) const noexcept { return (m_value & (uint32_t)capability) == (uint32_t)capability; }
	inline void AddCapability(const ECapability capability) noexcept { m_value = (m_value | capability); }
	inline ECapability GetCapability() const noexcept { return (ECapability)m_value; }

	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint32_t>(m_value);
	}

	static Capabilities Deserialize(ByteBuffer& byteBuffer)
	{
		const uint32_t value = byteBuffer.ReadU32();
		return Capabilities(value);
	}

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["bits"] = Json::UInt(m_value);
		return json;
	}

private:
	uint32_t m_value;
};