#include <catch.hpp>

#include <Crypto/BulletProofsCache.h>
#include <Crypto/Bulletproofs.h>
#include <Crypto/Models/Commitment.h>
#include <Crypto/Models/RangeProof.h>

TEST_CASE("Crypto::BulletproofsCacheBindsProofBytes")
{
	std::vector<unsigned char> commitmentBytes(33, 7);
	const Commitment commitment(CBigInteger<33>(std::move(commitmentBytes)));
	const RangeProof proofA(std::vector<unsigned char>(675, 1));
	const RangeProof proofB(std::vector<unsigned char>(675, 2));

	BulletProofsCache cache;
	REQUIRE_FALSE(cache.WasAlreadyVerified(commitment, proofA));

	cache.AddToCache(commitment, proofA);
	REQUIRE(cache.WasAlreadyVerified(commitment, proofA));

	// Same commitment, different proof: must not hit the cache.
	REQUIRE_FALSE(cache.WasAlreadyVerified(commitment, proofB));

	cache.AddToCache(commitment, proofB);
	REQUIRE(cache.WasAlreadyVerified(commitment, proofA));
	REQUIRE(cache.WasAlreadyVerified(commitment, proofB));
}

TEST_CASE("Crypto::BulletproofsRejectsEmptyBatchItem")
{
	std::vector<unsigned char> commitmentBytes(33, 9);
	const Commitment commitment(CBigInteger<33>(std::move(commitmentBytes)));

	REQUIRE(Bulletproofs::GetInstance().VerifyBulletproofs({}));

	std::vector<std::pair<Commitment, RangeProof>> invalid;
	invalid.emplace_back(commitment, RangeProof(std::vector<unsigned char>()));
	REQUIRE_FALSE(Bulletproofs::GetInstance().VerifyBulletproofs(invalid));
}