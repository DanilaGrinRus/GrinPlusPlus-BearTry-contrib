#include <catch.hpp>

#include <P2P/Seed/Seeder.h>

TEST_CASE("P2P accept decision matrix")
{
    REQUIRE(Seeder::ShouldAcceptPeer(false, 0, 10, false, true, false));
    REQUIRE(Seeder::ShouldAcceptPeer(false, 9, 10, false, true, false));

    REQUIRE_FALSE(Seeder::ShouldAcceptPeer(true, 0, 10, false, true, false));
    REQUIRE_FALSE(Seeder::ShouldAcceptPeer(false, 10, 10, false, true, false));
    REQUIRE_FALSE(Seeder::ShouldAcceptPeer(false, 0, 10, true, false, false));
    REQUIRE_FALSE(Seeder::ShouldAcceptPeer(false, 0, 10, false, false, false));
    REQUIRE_FALSE(Seeder::ShouldAcceptPeer(false, 0, 10, false, true, true));
}
