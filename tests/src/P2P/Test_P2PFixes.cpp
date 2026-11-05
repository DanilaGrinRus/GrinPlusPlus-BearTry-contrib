#include <catch.hpp>

#include <Core/Global.h>
#include <P2P/Pipeline/TxHashSetPipe.h>
#include <P2P/Seed/HandShake.h>

#include <sstream>
#include <string>

TEST_CASE("P2P handshake accepts only the local genesis hash")
{
    REQUIRE(HandShake::IsGenesisHashValid(Global::GetGenesisHash()));
    REQUIRE_FALSE(HandShake::IsGenesisHashValid(HASH::ZERO));
}

TEST_CASE("P2P archive send reads the final partial chunk")
{
    const std::string data(10000, 'x');
    std::istringstream input(data);
    char buffer[4096] = {};

    const std::streamsize first = TxHashSetPipe::ReadChunk(input, buffer, 4096);
    const std::streamsize second = TxHashSetPipe::ReadChunk(input, buffer, 4096);
    const std::streamsize third = TxHashSetPipe::ReadChunk(input, buffer, 4096);
    const std::streamsize fourth = TxHashSetPipe::ReadChunk(input, buffer, 4096);

    REQUIRE(first == 4096);
    REQUIRE(second == 4096);
    REQUIRE(third == 1808);
    REQUIRE(fourth == 0);
}
