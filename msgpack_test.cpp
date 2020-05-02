#include "catch.hpp"
#include "msgpack.h"

extern "C" {
#include "helloworld_msgpack.h"
#include "manykernels_msgpack.h"
}

#include <cstring>

TEST_CASE("str") { CHECK(helloworld_msgpack_len != 0); }

void on_matching_string_key_apply_action_to_value(
    byte_range bytes,
    std::function<bool(size_t N, const unsigned char *bytes)> predicate,
    std::function<void(byte_range)> action) {
  bool matched;

  functors f;

  f.cb_string = [&predicate, &matched](size_t N, const unsigned char *str) {
    if (predicate(N, str)) {
      matched = true;
    }
  };

  f.cb_map_elements = [&](byte_range key, byte_range value) {
    matched = false;
    const unsigned char *r = handle_msgpack(key, f);
    assert(r == value.start);
    if (matched) {
      action(value);
    }
  };

  handle_msgpack(bytes, f);
}

TEST_CASE("hello world") {

  SECTION("run it") {
    msgpack::dump(
        {helloworld_msgpack, helloworld_msgpack + helloworld_msgpack_len});
  }

  SECTION("build name : segment size map") {
    const unsigned char *kernels_start = nullptr;
    const unsigned char *kernels_end = nullptr;

    on_matching_string_key_apply_action_to_value(
        {helloworld_msgpack, helloworld_msgpack + helloworld_msgpack_len},
        [](size_t N, const unsigned char *bytes) {
          const char *ref = "amdhsa.kernels";
          size_t nref = strlen(ref);
          if (N == nref) {
            if (memcmp(bytes, ref, N) == 0) {
              return true;
            }
          }
          return false;
        },
        [&](byte_range bytes) {
          if (kernels_start == nullptr) {
            kernels_start = bytes.start;
            kernels_end = bytes.end;
          } else {
            printf("unhandled\n");
          }
        });

    if (kernels_start) {

      auto is_str = [](const char *key, uint64_t N,
                       const unsigned char *bytes) -> bool {
        if (strlen(key) == N) {
          if (memcmp(bytes, key, N) == 0) {
            return true;
          }
        }
        return false;
      };

      uint64_t segment_size = UINT64_MAX;
      std::string kernel_name = "";

      functors f;
      f.cb_array = [&](uint64_t N, byte_range bytes) -> const unsigned char * {
        bool hit_name = false;
        bool hit_segment_size = false;

        functors find_string_key;
        find_string_key.cb_string = [&](size_t N, const unsigned char *bytes) {
          if (is_str(".name", N, bytes)) {
            hit_name = true;
          }
          if (is_str(".kernarg_segment_size", N, bytes)) {
            hit_segment_size = true;
          }
        };

        functors get_uint;
        get_uint.cb_unsigned = [&](uint64_t x) { segment_size = x; };

        functors get_name;
        get_name.cb_string = [&](size_t N, const unsigned char *bytes) {
          kernel_name = std::string(bytes, bytes + N);
        };

        functors over_map;
        over_map.cb_map = [&](uint64_t N,
                              byte_range bytes) -> const unsigned char * {
          for (uint64_t i = 0; i < N; i++) {
            const unsigned char *start_key = bytes.start;

            hit_name = false;
            hit_segment_size = false;

            const unsigned char *end_key =
                handle_msgpack({start_key, bytes.end}, find_string_key);
            if (!end_key) {
              return 0;
            }

            const unsigned char *start_value = end_key;

            if (hit_name) {
              handle_msgpack({start_value, bytes.end}, get_name);
            }

            if (hit_segment_size) {
              handle_msgpack({start_value, bytes.end}, get_uint);
            }

            // Skip over the value
            const unsigned char *end_value =
            handle_msgpack({start_value, bytes.end}, functors());
            if (!end_value) {
              return 0;
            }
            bytes.start = end_value;
          }

          return bytes.start;
        };

        for (uint64_t i = 0; i < N; i++) {
          const unsigned char *next = handle_msgpack(bytes, over_map);
          if (!next) {
            return 0;
          }
          bytes.start = next;
        }
        return bytes.start;
      };

      handle_msgpack({kernels_start, kernels_end}, f);

      printf("Kernel %s has segment size %lu\n", kernel_name.c_str(),
             segment_size);
    }
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
            functors f;
            f.cb_unsigned = [&](uint64_t x) {
              kernarg_count++;
              kernarg_res = x;
            };
            handle_msgpack(value, f);
          }

          if (message_is_string(key, ".name")) {
            functors f;
            f.cb_string = [&](size_t N, const unsigned char *str) {
              kernname_count++;
              kernname_res = std::string(str, str + N);
            };
            handle_msgpack(value, f);
          }
        };

        foreach_map(bytes, inner);

        if (kernarg_count == 1 && kernname_count == 1) {
          // printf("winning\n");
        }
      });
    });
  }
}
