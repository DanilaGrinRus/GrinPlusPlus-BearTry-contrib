#include <catch.hpp>

#include <Core/Exceptions/DeserializationException.h>
#include <Core/Models/OutputIdentifier.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <PMMR/Common/Segment.h>

namespace
{
	// Segment is a template; the alias keeps commas out of Catch2 macros.
	using TestSegment = Segment<1, OutputIdentifier>;

	// A segment header followed by an arbitrary leaf count and no leaf payload.
	// The count must be rejected against the remaining payload size before any
	// leaf storage is reserved (G-20).
	ByteBuffer CraftLeafCountBuffer(const uint64_t numLeaves)
	{
		Serializer serializer;
		serializer.Append<uint8_t>(1);      // segment height
		serializer.Append<uint64_t>(0);     // segment index
		serializer.Append<uint64_t>(0);     // numHashes
		serializer.Append<uint64_t>(numLeaves);
		serializer.Append<uint64_t>(0);     // empty SegmentProof
		return ByteBuffer(serializer.GetBytes());
	}
}

TEST_CASE("Segment::DeserializeRejectsExcessiveLeafCount")
{
	ByteBuffer buffer = CraftLeafCountBuffer(1000);
	REQUIRE_THROWS_AS(TestSegment::Deserialize(buffer), DeserializationException);
}

TEST_CASE("Segment::DeserializeAcceptsEmptyLeafList")
{
	ByteBuffer buffer = CraftLeafCountBuffer(0);
	REQUIRE_NOTHROW(TestSegment::Deserialize(buffer));
}
