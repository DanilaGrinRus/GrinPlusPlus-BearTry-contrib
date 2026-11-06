#include <catch.hpp>

#include <Core/Models/SpentOutput.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Crypto/Models/Commitment.h>
#include <PMMR/Common/LeafIndex.h>

TEST_CASE("SpentOutputs Release-mode deserialize consumes version byte")
{
	const Commitment commitment(CBigInteger<33>::ValueOf(7));
	const OutputLocation location(LeafIndex::At(3), 3317928);
	const SpentOutputs original(std::vector<SpentOutput>({ SpentOutput(commitment, location) }));

	Serializer serializer;
	original.Serialize(serializer);

	// This is the Release/NDEBUG path: the version byte must be consumed
	// explicitly, not only by an assert that is compiled out.
	const std::vector<uint8_t>& bytes = serializer.GetBytes();
	ByteBuffer byteBuffer(bytes);
	const SpentOutputs decoded = SpentOutputs::Deserialize(byteBuffer);
	REQUIRE(decoded.GetSpentOutputs().size() == 1);

	const std::unordered_map<Commitment, OutputLocation> map = decoded.BuildMap();
	REQUIRE(map.size() == 1);
	REQUIRE(map.count(commitment) == 1);
	REQUIRE(map.at(commitment).GetLeafIndex().GetPosition() == location.GetLeafIndex().GetPosition());
	REQUIRE(map.at(commitment).GetBlockHeight() == location.GetBlockHeight());

	// A non-zero version is rejected in every build instead of being accepted
	// silently when NDEBUG compiles the old assert() out.
	std::vector<uint8_t> badBytes = bytes;
	badBytes[0] = 1;
	ByteBuffer badBuffer(badBytes);
	REQUIRE_THROWS_AS(SpentOutputs::Deserialize(badBuffer), DeserializationException);
}
