#include "catch.hpp"
#include "msgpack.h"

extern "C" {
#include "helloworld_msgpack.h"
#include "manykernels_msgpack.h"
}

#include <cstring>

TEST_CASE("str") { CHECK(helloworld_msgpack_len != 0); }

using namespace msgpack;

TEST_CASE("hello world") {

  SECTION("run it") {
    msgpack::dump(
        {helloworld_msgpack, helloworld_msgpack + helloworld_msgpack_len});
  }

  SECTION("run it bigger") {
    unsigned char *start = manykernels_msgpack;
    unsigned char *end = manykernels_msgpack + manykernels_msgpack_len;

    foreach_map({start, end}, [&](byte_range key, byte_range value) {
      bool matched = message_is_string(key, "amdhsa.kernels");

      if (!matched) {
        return;
      }

      foreach_array(value, [&](byte_range bytes) {
        uint64_t kernarg_count = 0;
        uint64_t kernarg_res;
        uint64_t kernname_count = 0;
        std::string kernname_res;

        auto inner = [&](byte_range key, byte_range value) {
          if (message_is_string(key, ".kernarg_segment_size")) {
            foronly_unsigned(value, [&](uint64_t x) {
              kernarg_count++;
              kernarg_res = x;
            });
          }

          if (message_is_string(key, ".name")) {
            foronly_string(value, [&](size_t N, const unsigned char *str) {
              kernname_count++;
              kernname_res = std::string(str, str + N);
            });
          }
        };

        foreach_map(bytes, inner);

        if (kernarg_count == 1 && kernname_count == 1) {
          printf("Kernel %s has segsize %lu\n", kernname_res.c_str(), kernarg_res);
        }
      });
    });
  }
}
