#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

extern "C" {
#include "helloworld_msgpack.h"
}

namespace msgpack {
typedef enum {
#define X(NAME, WIDTH, PAYLOAD) NAME,
#include "msgpack.def"
#undef X
} type;

const char *type_name(type ty) {
  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD)                                                \
  case NAME:                                                                   \
    return #NAME;
#include "msgpack.def"
#undef X
  }
}
} // namespace msgpack

[[noreturn]] void internal_error(void) {
  printf("internal error\n");
  exit(1);
}

msgpack::type parse_type(unsigned char x) {
  using namespace msgpack;
  switch (x) {
  case 0x00 ... 0x7f:
    return posfixint;
  case 0x80 ... 0x8f:
    return fixmap;
  case 0x90 ... 0x9f:
    return fixarray;
  case 0xa0 ... 0xbf:
    return fixstr;
  case 0xc0:
    return nil;
  case 0xc1:
    return never_used;
  case 0xc2:
    return f;
  case 0xc3:
    return t;
  case 0xc4:
    return bin8;
  case 0xc5:
    return bin16;
  case 0xc6:
    return bin32;
  case 0xc7:
    return ext8;
  case 0xc8:
    return ext16;
  case 0xc9:
    return ext32;
  case 0xca:
    return float32;
  case 0xcb:
    return float64;
  case 0xcc:
    return uint8;
  case 0xcd:
    return uint16;
  case 0xce:
    return uint32;
  case 0xcf:
    return uint64;
  case 0xd0:
    return int8;
  case 0xd1:
    return int16;
  case 0xd2:
    return int32;
  case 0xd3:
    return int64;
  case 0xd4:
    return fixext1;
  case 0xd5:
    return fixext2;
  case 0xd6:
    return fixext4;
  case 0xd7:
    return fixext8;
  case 0xd8:
    return fixext16;
  case 0xd9:
    return str8;
  case 0xda:
    return str16;
  case 0xdb:
    return str32;
  case 0xdc:
    return array16;
  case 0xdd:
    return array32;
  case 0xde:
    return map16;
  case 0xdf:
    return map32;
  case 0xe0 ... 0xff:
    return negfixint;

  default:
    internal_error();
  }
}

template <typename T, typename R> R bitcast(T x) {
  static_assert(sizeof(T) == sizeof(R), "");
  R tmp;
  memcpy(&tmp, &x, sizeof(T));
  return tmp;
}

// Helper functions for reading additional payload from the header
// Depending on the type, this can be a number of bytes, elements,
// key-value pairs or an embedded integer.
// Each takes a pointer to the start of the header and returns a uint64_t

typedef uint64_t (*payload_info_t)(unsigned char *);

// Some types don't contain embedded information, e.g. true
// boolean may be available as a mask
uint64_t read_zero(unsigned char *) { return 0; }
uint64_t read_one(unsigned char *) { return 1; }

// Read the first byte and zero/sign extend it
uint64_t read_embedded_u8(unsigned char *start) { return start[0]; }
uint64_t read_embedded_s8(unsigned char *start) {
  int64_t res = bitcast<uint8_t, int8_t>(start[0]);
  return bitcast<int64_t, uint64_t>(res);
}

// Read a masked part of the first byte
uint64_t read_via_mask_0xf(unsigned char *start) { return *start & 0xfu; }
uint64_t read_via_mask_0x1f(unsigned char *start) { return *start & 0x1fu; }

// Read 1/2/4/8 bytes immediately following the type byte and zero/sign extend
// Big endian format.
uint64_t read_size_field_u8(unsigned char *from) {
  from++;
  return from[0];
}
uint64_t read_size_field_u16(unsigned char *from) {
  from++;
  return (from[0] << 8u) | from[1];
}
uint64_t read_size_field_u32(unsigned char *from) {
  from++;
  return (from[0] << 24u) | (from[1] << 16u) | (from[2] << 8u) |
         (from[3] << 0u);
}
uint64_t read_size_field_u64(unsigned char *from) {
  from++;
  return ((uint64_t)from[0] << 56u) | ((uint64_t)from[1] << 48u) |
         ((uint64_t)from[2] << 40u) | ((uint64_t)from[3] << 32u) |
         (from[4] << 24u) | (from[5] << 16u) | (from[6] << 8u) |
         (from[7] << 0u);
}

uint64_t read_size_field_s8(unsigned char *from) {
  uint8_t u = read_size_field_u8(from);
  int64_t res = bitcast<uint8_t, int8_t>(u);
  return bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s16(unsigned char *from) {
  uint16_t u = read_size_field_u16(from);
  int64_t res = bitcast<uint16_t, int16_t>(u);
  return bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s32(unsigned char *from) {
  uint32_t u = read_size_field_u32(from);
  int64_t res = bitcast<uint32_t, int32_t>(u);
  return bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s64(unsigned char *from) {
  uint64_t u = read_size_field_u64(from);
  int64_t res = bitcast<uint64_t, int64_t>(u);
  return bitcast<int64_t, uint64_t>(res);
}

payload_info_t payload_info(msgpack::type ty) {
  using namespace msgpack;
  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD)                                                \
  case NAME:                                                                   \
    return PAYLOAD;
#include "msgpack.def"
#undef X
  }
}

// Only failure mode is going to be out of bounds
// Return NULL on out of bounds, otherwise start of the next entry

unsigned bytes_used_fixed(msgpack::type ty) {
  using namespace msgpack;
  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD)                                                \
  case NAME:                                                                   \
    return WIDTH;
#include "msgpack.def"
#undef X
  };
}

struct functors {
  functors();

  std::function<void(size_t, unsigned char *)> cb_string =
      [](size_t, unsigned char *) {};

  std::function<void(int64_t)> cb_signed = [](int64_t) {};

  std::function<void(uint64_t)> cb_unsigned = [](uint64_t) {};

  std::function<void(bool)> cb_boolean = [](bool) {};

  std::function<unsigned char *(uint64_t N, unsigned char *start,
                                unsigned char *end)>
      cb_array;

  std::function<unsigned char *(uint64_t N, unsigned char *start,
                                unsigned char *end)>
      cb_map;
};

unsigned char *handle_msgpack(unsigned char *start, unsigned char *end,
                              functors f);

functors::functors()
    : cb_array{[](uint64_t N, unsigned char *start,
                  unsigned char *end) -> unsigned char * {
        for (uint64_t i = 0; i < N; i++) {
          unsigned char *next = handle_msgpack(start, end, {});
          if (!next) {
            return 0;
          }
          start = next;
        }
        return start;
      }},
      cb_map{[](uint64_t N, unsigned char *start,
                unsigned char *end) -> unsigned char * {
        for (uint64_t i = 0; i < 2 * N; i++) {
          unsigned char *next = handle_msgpack(start, end, {});
          if (!next) {
            return 0;
          }
          start = next;
        }
        return start;
      }} {}

unsigned char *
handle_str(unsigned char *start, unsigned char *end,
           std::function<void(size_t, unsigned char *)> callback) {
  uint64_t available = end - start;
  assert(available != 0);

  msgpack::type ty = parse_type(*start);
  uint64_t bytes = bytes_used_fixed(ty);

  if (available < bytes) {
    return 0;
  }
  uint64_t available_post_header = available - bytes;

  payload_info_t info = payload_info(ty);
  uint64_t N = info(start);
  switch (ty) {
  case msgpack::fixstr: {
    N = read_via_mask_0x1f(start);
    assert(N == info(start));
    break;
  }
  case msgpack::str8: {
    N = read_size_field_u8(start);
    assert(N == info(start));
    break;
  }
  case msgpack::str16: {
    N = read_size_field_u16(start);
    assert(N == info(start));
    break;
  }
  case msgpack::str32: {
    N = read_size_field_u32(start);
    assert(N == info(start));
    break;
  }

  default:
    internal_error();
  }

  if (available_post_header < N) {
    return 0;
  }
  callback(N, start + bytes);
  return start + bytes + N;
}

unsigned char *handle_boolean(unsigned char *start, unsigned char *end,
                              std::function<void(bool)> callback) {
  uint64_t available = end - start;
  assert(available != 0);
  msgpack::type ty = parse_type(*start);
  payload_info_t info = payload_info(ty);
  uint64_t N = info(start);
  switch (ty) {
  case msgpack::t: {
    N = 1;
    assert(N == info(start));
    break;
  }
  case msgpack::f: {
    N = 0;
    assert(N == info(start));
    break;
  }

  default:
    internal_error();
  }

  callback(!!N);
  assert(bytes_used_fixed(ty) == 1);
  return start + 1;
}

unsigned char *handle_uint(unsigned char *start, unsigned char *end,
                           std::function<void(uint64_t)> unsigned_callback

) {
  uint64_t available = end - start;
  assert(available != 0);

  msgpack::type ty = parse_type(*start);
  uint64_t bytes = bytes_used_fixed(ty);

  if (available < bytes) {
    return 0;
  }

  payload_info_t info = payload_info(ty);
  uint64_t N = info(start);
  switch (ty) {
  case msgpack::posfixint: {
    // considered 'unsigned' by spec
    N = read_embedded_u8(start);
    assert(N == info(start));
    break;
  }

  case msgpack::uint8: {
    N = read_size_field_u8(start);
    assert(N == info(start));
    break;
  }
  case msgpack::uint16: {
    N = read_size_field_u16(start);
    assert(N == info(start));
    break;
  }
  case msgpack::uint32: {
    N = read_size_field_u32(start);
    assert(N == info(start));
    break;
  }
  case msgpack::uint64: {
    N = read_size_field_u64(start);
    assert(N == info(start));
    break;
  }

  default:
    internal_error();
  }

  unsigned_callback(N);
  return start + bytes;
}

unsigned char *handle_sint(unsigned char *start, unsigned char *end,
                           std::function<void(int64_t)> signed_callback

) {
  uint64_t available = end - start;
  assert(available != 0);

  msgpack::type ty = parse_type(*start);
  uint64_t bytes = bytes_used_fixed(ty);

  if (available < bytes) {
    return 0;
  }

  payload_info_t info = payload_info(ty);
  uint64_t N = info(start);
  switch (ty) {
  case msgpack::negfixint: {
    // considered 'signed' by spec
    N = read_embedded_s8(start);
    assert(N == info(start));
    break;
  }
  case msgpack::int8: {
    N = read_size_field_s8(start);
    assert(N == info(start));
    break;
  }
  case msgpack::int16: {
    N = read_size_field_s16(start);
    assert(N == info(start));
    break;
  }
  case msgpack::int32: {
    N = read_size_field_s32(start);
    assert(N == info(start));
    break;
  }
  case msgpack::int64: {
    N = read_size_field_s64(start);
    assert(N == info(start));
    break;
  }
  default:
    internal_error();
  }

  signed_callback(bitcast<uint64_t, int64_t>(N));

  return start + bytes;
}

unsigned char *
handle_array(unsigned char *start, unsigned char *end,
             std::function<unsigned char *(uint64_t N, unsigned char *start,
                                           unsigned char *end)>
                 callback) {
  uint64_t available = end - start;
  assert(available != 0);

  msgpack::type ty = parse_type(*start);
  uint64_t bytes = bytes_used_fixed(ty);

  if (available < bytes) {
    return 0;
  }

  payload_info_t info = payload_info(ty);
  uint64_t N = info(start);
  switch (ty) {
  case msgpack::fixarray: {
    N = read_via_mask_0xf(start);
    assert(N == info(start));
    break;
  }

  case msgpack::array16: {
    N = read_size_field_u16(start);
    assert(N == info(start));
    break;
  }

  case msgpack::array32: {
    N = read_size_field_u32(start);
    assert(N == info(start));
    break;
  }

  default:
    internal_error();
  }

  return callback(N, start + bytes, end);
}

unsigned char *
handle_map(unsigned char *start, unsigned char *end,
           std::function<unsigned char *(uint64_t N, unsigned char *start,
                                         unsigned char *end)>
               callback) {

  uint64_t available = end - start;
  assert(available != 0);

  msgpack::type ty = parse_type(*start);
  uint64_t bytes = bytes_used_fixed(ty);

  if (available < bytes) {
    return 0;
  }

  payload_info_t info = payload_info(ty);
  uint64_t N = info(start);
  switch (ty) {
  case msgpack::fixmap: {
    N = read_via_mask_0xf(start);
    assert(N == info(start));
    break;
  }
  case msgpack::map16: {
    N = read_size_field_u16(start);
    assert(N == info(start));
    break;
  }
  case msgpack::map32: {
    N = read_size_field_u32(start);
    assert(N == info(start));
    break;
  }
  default:
    internal_error();
  }
  return callback(N, start + bytes, end);
}

unsigned char *handle_unimplemented(unsigned char *start, unsigned char *end) {
  uint64_t available = end - start;
  assert(available != 0);
  msgpack::type ty = parse_type(*start);

  uint64_t bytes = bytes_used_fixed(ty);
  if (available < bytes) {
    return 0;
  }
  uint64_t available_post_header = available - bytes;

  payload_info_t info = payload_info(ty);
  uint64_t N = info(start);
  switch (ty) {
  case msgpack::nil:
  case msgpack::never_used:
  case msgpack::float32:
  case msgpack::float64:
  case msgpack::fixext1:
  case msgpack::fixext2:
  case msgpack::fixext4:
  case msgpack::fixext8:
  case msgpack::fixext16: {
    N = 0;
    assert(N == info(start));
    break;
  }
  case msgpack::ext8:
  case msgpack::bin8: {
    N = read_size_field_u8(start);
    assert(N == info(start));
    break;
  }
  case msgpack::ext16:
  case msgpack::bin16: {
    N = read_size_field_u16(start);
    assert(N == info(start));
    break;
  }
  case msgpack::ext32:
  case msgpack::bin32: {
    N = read_size_field_u32(start);
    assert(N == info(start));
    break;
  }
  default:
    internal_error();
  }

  if (available_post_header < N) {
    return 0;
  }

  return start + bytes + N;
}

unsigned char *handle_msgpack(unsigned char *start, unsigned char *end,
                              functors f) {
  uint64_t available = end - start;
  if (available == 0) {
    return 0;
  }
  msgpack::type ty = parse_type(*start);

  switch (ty) {
  case msgpack::t:
  case msgpack::f:
    return handle_boolean(start, end, f.cb_boolean);

  case msgpack::posfixint:
  case msgpack::uint8:
  case msgpack::uint16:
  case msgpack::uint32:
  case msgpack::uint64:
    return handle_uint(start, end, f.cb_unsigned);

  case msgpack::negfixint:
  case msgpack::int8:
  case msgpack::int16:
  case msgpack::int32:
  case msgpack::int64:
    return handle_sint(start, end, f.cb_signed);

  case msgpack::fixstr:
  case msgpack::str8:
  case msgpack::str16:
  case msgpack::str32:
    return handle_str(start, end, f.cb_string);

  case msgpack::fixarray:
  case msgpack::array16:
  case msgpack::array32:
    return handle_array(start, end, f.cb_array);

  case msgpack::fixmap:
  case msgpack::map16:
  case msgpack::map32:
    return handle_map(start, end, f.cb_map);

  case msgpack::nil:
  case msgpack::bin8:
  case msgpack::bin16:
  case msgpack::bin32:
  case msgpack::float32:
  case msgpack::float64:
  case msgpack::ext8:
  case msgpack::ext16:
  case msgpack::ext32:
  case msgpack::fixext1:
  case msgpack::fixext2:
  case msgpack::fixext4:
  case msgpack::fixext8:
  case msgpack::fixext16:
  case msgpack::never_used:
    return handle_unimplemented(start, end);
  }
}

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
    printf("%*s[\n", indent, "");
    indent += by;

    const char *sep = "";
    for (uint64_t i = 0; i < N; i++) {
      indent += by;
      printf("%s", sep);
      sep = ",";
      unsigned char *next = handle_msgpack(start, end, f);
      indent -= by;
      start = next;
      if (!next) {
        break;
      }
    }
    indent -= by;
    printf("%*s]\n", indent, "");

    return start;
  };

  f.cb_map = [&](uint64_t N, unsigned char *start,
                 unsigned char *end) -> unsigned char * {
    printf("%*s{\n", indent, "");
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
    printf("%*s}\n", indent, "");

    return start;
  };

  handle_msgpack(start, end, f);
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

  f.cb_map = [&](uint64_t N, unsigned char *start,
                 unsigned char *end) -> unsigned char * {
    for (uint64_t i = 0; i < N; i++) {
      matched = false;
      unsigned char *start_key = start;
      unsigned char *end_key = handle_msgpack(start_key, end, f);
      if (!end_key) {
        return 0;
      }

      if (matched) {
        action(end_key, end);
      }

      // Skip over the value
      unsigned char *start_value = end_key;
      unsigned char *end_value = handle_msgpack(start_value, end, f);
      if (!end_value) {
        return 0;
      }
      start = end_value;
    }
    return start;
  };

  handle_msgpack(start, end, f);
}

TEST_CASE("hello world") {
  SECTION("sanity checks") {
    unsigned char start = helloworld_msgpack[0];
    CHECK(parse_type(start) == msgpack::fixmap);
  }

  SECTION("run it") {
    // json_print(helloworld_msgpack, helloworld_msgpack +
    // helloworld_msgpack_len);
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
}
