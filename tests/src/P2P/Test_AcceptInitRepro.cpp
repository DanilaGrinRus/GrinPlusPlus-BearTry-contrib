#include <catch.hpp>
#include <P2P/Seed/AcceptLoop.h>
#include <asio.hpp>
#include <cstring>
#include <new>

TEST_CASE("AcceptLoop Stop on freshly allocated nonzero memory")
{
    asio::io_service io;
    void* raw = ::operator new(sizeof(AcceptLoop));
    std::memset(raw, 0xa5, sizeof(AcceptLoop));
    AcceptLoop* loop = new(raw) AcceptLoop(io, 0);
    loop->Stop();
    loop->~AcceptLoop();
    ::operator delete(raw);
    SUCCEED();
}
