#include "catch.hpp"

extern "C" {
#include "helloworld_msgpack.h"
}

TEST_CASE("str") { CHECK(helloworld_msgpack_len != 0); }
