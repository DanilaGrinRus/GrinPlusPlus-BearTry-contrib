#include <catch.hpp>

#include <P2P/Pipeline/TxHashSetPipe.h>

#include <sstream>
#include <string>

TEST_CASE("Archive send reads the final partial chunk")
{
    const std::string data(10000, 'x');
    std::istringstream input(data);
    char buffer[4096] = {};

    REQUIRE(TxHashSetPipe::ReadChunk(input, buffer, 4096) == 4096);
    REQUIRE(TxHashSetPipe::ReadChunk(input, buffer, 4096) == 4096);
    REQUIRE(TxHashSetPipe::ReadChunk(input, buffer, 4096) == 1808);
    REQUIRE(TxHashSetPipe::ReadChunk(input, buffer, 4096) == 0);
}
