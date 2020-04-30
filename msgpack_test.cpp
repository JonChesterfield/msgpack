#include "catch.hpp"
#include "msgpack.hpp"

extern "C" {
#include "helloworld_msgpack.h"
#include "manykernels_msgpack.h"
}

#include <cstring>

TEST_CASE("str") { CHECK(helloworld_msgpack_len != 0); }

void json_print(unsigned char *start, unsigned char *end) {
  functors f;
  unsigned indent = 0;
  const unsigned by = 2;

  f.cb_string = [&](size_t N, unsigned char *bytes) {
    char *tmp = (char *)malloc(N + 1);
    memcpy(tmp, bytes, N);
    tmp[N] = '\0';
    printf("\"%s\"", tmp);
    free(tmp);
  };

  f.cb_signed = [&](int64_t x) { printf("%ld", x); };
  f.cb_unsigned = [&](uint64_t x) { printf("%lu", x); };

  f.cb_array = [&](uint64_t N, unsigned char *start,
                   unsigned char *end) -> unsigned char * {
    printf("\n%*s[\n", indent, "");
    indent += by;

    for (uint64_t i = 0; i < N; i++) {
      indent += by;
      printf("%*s", indent, "");
      unsigned char *next = handle_msgpack(start, end, f);
      printf(",\n");
      indent -= by;
      start = next;
      if (!next) {
        break;
      }
    }
    indent -= by;
    printf("%*s]", indent, "");

    return start;
  };

  f.cb_map = [&](uint64_t N, unsigned char *start,
                 unsigned char *end) -> unsigned char * {
    printf("\n%*s{\n", indent, "");
    indent += by;

    for (uint64_t i = 0; i < 2 * N; i += 2) {
      unsigned char *start_key = start;
      printf("%*s", indent, "");
      unsigned char *end_key = handle_msgpack(start_key, end, f);
      if (!end_key) {
        break;
      }

      printf(" : ");

      unsigned char *start_value = end_key;
      unsigned char *end_value = handle_msgpack(start_value, end, f);

      if (!end_value) {
        break;
      }

      printf(",\n");

      start = end_value;
    }

    indent -= by;
    printf("%*s}", indent, "");

    return start;
  };

  handle_msgpack(start, end, f);
  printf("\n");
}

void on_matching_string_key_apply_action_to_value(
    unsigned char *start, unsigned char *end,
    std::function<bool(size_t N, unsigned char *bytes)> predicate,
    std::function<void(unsigned char *start, unsigned char *end)> action) {
  bool matched;

  functors f;

  f.cb_string = [&predicate, &matched](size_t N, unsigned char *bytes) {
    if (predicate(N, bytes)) {
      matched = true;
    }
  };

  f.cb_map_elements = [&](unsigned char *start_key, unsigned char *end_key,
                          unsigned char *start_value,
                          unsigned char *end_value) {
    matched = false;
    unsigned char *r = handle_msgpack(start_key, end_key, f);
    assert(r == start_value);
    if (matched) {
      action(start_value, end_value);
    }
  };

  handle_msgpack(start, end, f);
}

bool message_is_string(unsigned char *start, unsigned char *end,
                       const char *str) {
  bool matched = false;
  functors f;
  size_t strN = strlen(str);

  f.cb_string = [=, &matched](size_t N, unsigned char *bytes) {
    if (N == strN) {
      if (memcmp(bytes, str, N) == 0) {
        matched = true;
      }
    }
  };

  handle_msgpack(start, end, f);
  return matched;
}

TEST_CASE("hello world") {

  SECTION("run it") {
    json_print(helloworld_msgpack, helloworld_msgpack + helloworld_msgpack_len);
  }

  SECTION("build name : segment size map") {
    unsigned char *kernels_start = nullptr;
    unsigned char *kernels_end = nullptr;

    on_matching_string_key_apply_action_to_value(
        helloworld_msgpack, helloworld_msgpack + helloworld_msgpack_len,
        [](size_t N, unsigned char *bytes) {
          const char *ref = "amdhsa.kernels";
          size_t nref = strlen(ref);
          if (N == nref) {
            if (memcmp(bytes, ref, N) == 0) {
              return true;
            }
          }
          return false;
        },
        [&](unsigned char *start, unsigned char *end) {
          if (kernels_start == nullptr) {
            kernels_start = start;
            kernels_end = end;
          } else {
            printf("unhandled\n");
          }
        });

    if (kernels_start) {

      auto is_str = [](const char *key, uint64_t N,
                       unsigned char *bytes) -> bool {
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
      f.cb_array = [&](uint64_t N, unsigned char *start,
                       unsigned char *end) -> unsigned char * {
        bool hit_name = false;
        bool hit_segment_size = false;

        functors find_string_key;
        find_string_key.cb_string = [&](size_t N, unsigned char *bytes) {
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
        get_name.cb_string = [&](size_t N, unsigned char *bytes) {
          kernel_name = std::string(bytes, bytes + N);
        };

        functors over_map;
        over_map.cb_map = [&](uint64_t N, unsigned char *start,
                              unsigned char *end) -> unsigned char * {
          for (uint64_t i = 0; i < N; i++) {
            unsigned char *start_key = start;

            hit_name = false;
            hit_segment_size = false;

            unsigned char *end_key =
                handle_msgpack(start_key, end, find_string_key);
            if (!end_key) {
              return 0;
            }

            unsigned char *start_value = end_key;

            if (hit_name) {
              handle_msgpack(start_value, end, get_name);
            }

            if (hit_segment_size) {
              handle_msgpack(start_value, end, get_uint);
            }

            // Skip over the value
            unsigned char *end_value = handle_msgpack(start_value, end, {});
            if (!end_value) {
              return 0;
            }
            start = end_value;
          }

          return start;
        };

        for (uint64_t i = 0; i < N; i++) {
          unsigned char *next = handle_msgpack(start, end, over_map);
          if (!next) {
            return 0;
          }
          start = next;
        }
        return start;
      };

      handle_msgpack(kernels_start, kernels_end, f);

      printf("Kernel %s has segment size %lu\n", kernel_name.c_str(),
             segment_size);
    }
  }

  SECTION("run it big") {
    unsigned char *start = manykernels_msgpack;
    unsigned char *end = manykernels_msgpack + manykernels_msgpack_len;

    foreach_map(start, end,
                [&](unsigned char *start_key, unsigned char *end_key,
                    unsigned char *start_value, unsigned char *end_value) {
                  bool matched =
                      message_is_string(start_key, end_key, "amdhsa.kernels");

                  if (!matched) {
                    return;
                  }

                  foreach_array(
                      start_value, end_value,
                      [&](unsigned char *start, unsigned char *end) {
                        uint64_t kernarg_count = 0;
                        uint64_t kernarg_res;
                        uint64_t kernname_count = 0;
                        std::string kernname_res;

                        auto inner = [&](unsigned char *start_key,
                                         unsigned char *end_key,
                                         unsigned char *start_value,
                                         unsigned char *end_value) {
                          if (message_is_string(start_key, end_key,
                                                ".kernarg_segment_size")) {
                            functors f;
                            f.cb_unsigned = [&](uint64_t x) {
                              kernarg_count++;
                              kernarg_res = x;
                            };
                            handle_msgpack(start_value, end_value, f);
                          }

                          if (message_is_string(start_key, end_key, ".name")) {
                            functors f;
                            f.cb_string = [&](size_t N, unsigned char *bytes) {
                              kernname_count++;
                              kernname_res = std::string(bytes, bytes + N);
                            };
                            handle_msgpack(start_value, end_value, f);
                          }
                        };

                        foreach_map(start, end, inner);

                        if (kernarg_count == 1 && kernname_count == 1) {
                          printf("winning\n");
                        }
                      });
                });
  }
}
