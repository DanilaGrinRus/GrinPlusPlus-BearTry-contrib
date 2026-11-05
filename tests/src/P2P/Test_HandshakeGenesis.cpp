#include <catch.hpp>

#include <Core/Global.h>
#include <P2P/Seed/HandShake.h>

TEST_CASE("Handshake accepts only the local genesis hash")
{
    REQUIRE(HandShake::IsGenesisHashValid(Global::GetGenesisHash()));
    REQUIRE_FALSE(HandShake::IsGenesisHashValid(HASH::ZERO));
}
