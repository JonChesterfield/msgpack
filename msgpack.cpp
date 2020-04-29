#include "catch.hpp"

#include <cstdint>
#include <cstring>

extern "C" {
#include "helloworld_msgpack.h"
}

namespace msgpack {
typedef enum {
  posfixint,
  fixmap,
  fixarray,
  fixstr,
  nil,
  f,
  t,
  bin8,
  bin16,
  bin32,
  ext8,
  ext16,
  ext32,
  float32,
  float64,
  uint8,
  uint16,
  uint32,
  uint64,
  int8,
  int16,
  int32,
  int64,
  fixext1,
  fixext2,
  fixext4,
  fixext8,
  fixext16,
  str8,
  str16,
  str32,
  array16,
  array32,
  map16,
  map32,
  negfixint,
  failure,
} type;

const char *type_name(type ty) {
  switch (ty) {
  case posfixint:
    return "posfixint";
  case fixmap:
    return "fixmap";
  case fixarray:
    return "fixarray";
  case fixstr:
    return "fixstr";
  case nil:
    return "nil";
  case f:
    return "f";
  case t:
    return "t";
  case bin8:
    return "bin8";
  case bin16:
    return "bin16";
  case bin32:
    return "bin32";
  case ext8:
    return "ext8";
  case ext16:
    return "ext16";
  case ext32:
    return "ext32";
  case float32:
    return "float32";
  case float64:
    return "float64";
  case uint8:
    return "uint8";
  case uint16:
    return "uint16";
  case uint32:
    return "uint32";
  case uint64:
    return "uint64";
  case int8:
    return "int8";
  case int16:
    return "int16";
  case int32:
    return "int32";
  case int64:
    return "int64";
  case fixext1:
    return "fixext1";
  case fixext2:
    return "fixext2";
  case fixext4:
    return "fixext4";
  case fixext8:
    return "fixext8";
  case fixext16:
    return "fixext16";
  case str8:
    return "str8";
  case str16:
    return "str16";
  case str32:
    return "str32";
  case array16:
    return "array16";
  case array32:
    return "array32";
  case map16:
    return "map16";
  case map32:
    return "map32";
  case negfixint:
    return "negfixint";
  case failure:
    return "failure";
  }
}
} // namespace msgpack

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
    return failure;
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
    return failure;
  }
}

[[noreturn]] void internal_error(void) {
  printf("internal error\n");
  exit(1);
}

uint8_t read_bigendian_u8(unsigned char *from) { return from[0]; }
uint16_t read_bigendian_u16(unsigned char *from) {

  return (from[0] << 8u) | from[1];
}

uint32_t read_bigendian_u32(unsigned char *from) {

  return (from[0] << 24u) | (from[1] << 16u) | (from[2] << 8u) |
         (from[3] << 0u);
}

uint32_t read_bigendian_u64(unsigned char *from) {

  return ((uint64_t)from[0] << 56u) | ((uint64_t)from[1] << 48u) |
         ((uint64_t)from[2] << 40u) | ((uint64_t)from[3] << 32u) |
         (from[4] << 24u) | (from[5] << 16u) | (from[6] << 8u) |
         (from[7] << 0u);
}

int8_t read_bigendian_s8(unsigned char *from) {
  uint8_t u = read_bigendian_u8(from);
  int8_t s;
  memcpy(&s, &u, 1);
  return s;
}
int16_t read_bigendian_s16(unsigned char *from) {
  uint16_t u = read_bigendian_u16(from);
  int16_t s;
  memcpy(&s, &u, 2);
  return s;
}
int32_t read_bigendian_s32(unsigned char *from) {
  uint32_t u = read_bigendian_u32(from);
  int32_t s;
  memcpy(&s, &u, 4);
  return s;
}
int64_t read_bigendian_s64(unsigned char *from) {
  uint64_t u = read_bigendian_u64(from);
  int64_t s;
  memcpy(&s, &u, 8);
  return s;
}

// Only failure mode is going to be out of bounds
// Return NULL on out of bounds, otherwise start of the next entry
unsigned char *handle_msgpack(unsigned char *start, unsigned char *end);

unsigned char *handle_str_data(uint64_t N, unsigned char *start,
                               unsigned char *end) {

  char *tmp = (char *)malloc(N + 1);
  memcpy(tmp, start, N);
  tmp[N] = '\0';
  printf("got a string: %s\n", tmp);
  free(tmp);

  return start + N;
}

unsigned char *handle_str(unsigned char *start, unsigned char *end) {
  uint64_t available = end - start;
  assert(available != 0);

  switch (parse_type(*start)) {
  case msgpack::fixstr: {
    uint64_t N = *start & 0x1fu;
    return handle_str_data(N, start + 1, end);
  }
  case msgpack::str8: {
    if (available < 2) { return 0; }
    uint64_t N = read_bigendian_u8(start+1);
    return handle_str_data(N, start + 2, end);
  }
  case msgpack::str16: {
    if (available < 3) { return 0; }
    uint64_t N = read_bigendian_u16(start+1);
    return handle_str_data(N, start + 3, end);
  }
  case msgpack::str32: {
    if (available < 5) { return 0; }
    uint64_t N = read_bigendian_u32(start+1);
    return handle_str_data(N, start + 5, end);
  }
    
  default:
    internal_error();
  }
}

void sink_sint(int64_t x) { printf("sint got %ld\n", x); }
void sink_uint(uint64_t x) { printf("uint got %lu\n", x); }

unsigned bytes_used(msgpack::type ty)
{
  using namespace msgpack;
  switch(ty)
    {
    case negfixint: return 1;
    case int8: return 2;
    case int16: return 3;
    case int32: return 5;
    case int64: return 9;

    case posfixint: return 1;
    case uint8: return 2;
    case uint16: return 3;
    case uint32: return 5;
    case uint64: return 9;
      
    default:
      printf("Unimplemented bytes_used for %s\n", type_name(ty));
      internal_error();
    }
}
unsigned char *handle_int(unsigned char *start, unsigned char *end) {
  uint64_t available = end - start;
  assert(available != 0);

  msgpack::type ty = parse_type(*start);
  uint64_t bytes = bytes_used(ty);

  if (available < bytes) {
    return 0;
  }
  
  switch (ty) {
  case msgpack::posfixint: {
    // considered 'unsigned' by spec
    sink_uint(read_bigendian_s8(start));
    return start + bytes;
  }
  case msgpack::negfixint: {
    // considered 'signed' by spec
    sink_sint(read_bigendian_s8(start));
    return start + bytes;
  }
  case msgpack::int8: {
    sink_sint(read_bigendian_s8(start + 1));
    return start + bytes;
  }
  case msgpack::int16: {
    sink_sint(read_bigendian_s16(start + 1));
    return start + bytes;
  }
  case msgpack::int32: {
    sink_sint(read_bigendian_s32(start + 1));
    return start + bytes;
  }
  case msgpack::int64: {
    sink_sint(read_bigendian_s64(start + 1));
    return start + bytes;
  }

  case msgpack::uint8: {
    sink_uint(read_bigendian_u8(start + 1));
    return start + bytes;
  }
  case msgpack::uint16: {
    sink_uint(read_bigendian_u16(start + 1));
    return start + bytes;
  }
  case msgpack::uint32: {
    sink_uint(read_bigendian_u32(start + 1));
    return start + bytes;
  }
  case msgpack::uint64: {
    sink_uint(read_bigendian_u64(start + 1));
    return start + bytes;
  }

  default:
    internal_error();
  }
}
unsigned char *handle_array_elements(uint64_t N, unsigned char *start,
                                     unsigned char *end) {

  uint64_t available = end - start;
  printf("%lu array elements in %lu bytes\n", N, available);

  for (uint64_t i = 0; i < N; i++) {
    printf("Array element %lu: %s\n", i, type_name(parse_type(*start)));
    start = handle_msgpack(start, end);
    if (!start) {
      return 0;
    }
  }

  return start;
}

unsigned char *handle_array(unsigned char *start, unsigned char *end) {
  uint64_t available = end - start;
  assert(available != 0);

  // very similar to map
  switch (parse_type(*start)) {
  case msgpack::fixarray: {
    uint64_t N = *start & 0xfu;
    return handle_array_elements(N, start + 1, end);
  }

  case msgpack::array16: {
    if (available < 3) {
      return 0;
    }
    uint64_t N = read_bigendian_u16(start + 1);
    return handle_array_elements(N, start + 3, end);
  }

  case msgpack::array32: {
    if (available < 5) {
      return 0;
    }
    uint64_t N = read_bigendian_u32(start + 1);
    return handle_array_elements(N, start + 5, end);
  }

  default:
    internal_error();
  }
}

unsigned char *handle_map_elements(uint64_t N, unsigned char *start,
                                   unsigned char *end) {

  uint64_t available = end - start;
  printf("%lu map pairs in %lu bytes\n", N, available);

  for (uint64_t i = 0; i < 2 * N; i++) {
    printf("Map element %lu: %s\n", i, type_name(parse_type(*start)));

    // Probably want to pass element + next one to a function together
    start = handle_msgpack(start, end);
    if (!start) {
      return 0;
    }
  }

  return start;
}

unsigned char *handle_map(unsigned char *start, unsigned char *end) {
  uint64_t available = end - start;
  assert(available != 0);

  switch (parse_type(*start)) {
  case msgpack::fixmap: {
    uint64_t N = *start & 0xfu;
    return handle_map_elements(N, start + 1, end);
  }
  case msgpack::map16: {
    if (available < 3) {
      return 0;
    }
    uint64_t N = read_bigendian_u16(start + 1);
    return handle_map_elements(N, start + 3, end);
  }
  case msgpack::map32: {
    if (end - start < 5) {
      return 0;
    }
    uint64_t N = read_bigendian_u32(start + 1);
    return handle_map_elements(N, start + 5, end);
  }
  default:
    internal_error();
  }
}

unsigned char *handle_msgpack(unsigned char *start, unsigned char *end) {
  uint64_t available = end - start;
  if (available == 0) {
    return 0;
  }

  switch (parse_type(*start)) {
  case msgpack::fixmap:
  case msgpack::map16:
  case msgpack::map32:
    return handle_map(start, end);

  case msgpack::fixarray:
  case msgpack::array16:
  case msgpack::array32:
    return handle_array(start, end);

  case msgpack::fixstr:
  case msgpack::str8:
  case msgpack::str16:
  case msgpack::str32:
    return handle_str(start, end);


  case msgpack::posfixint:
  case msgpack::int8:
  case msgpack::int16:
  case msgpack::int32:
  case msgpack::int64:
  case msgpack::negfixint:
  case msgpack::uint8:
  case msgpack::uint16:
  case msgpack::uint32:
  case msgpack::uint64:
    return handle_int(start, end);
    
  default:
    printf("Unimplemented handler for %s\n", type_name(parse_type(*start)));
    internal_error();
  }
}

TEST_CASE("str") { CHECK(helloworld_msgpack_len != 0); }

TEST_CASE("hello world") {
  SECTION("sanity checks") {
    unsigned char start = helloworld_msgpack[0];
    CHECK(parse_type(start) == msgpack::fixmap);
    handle_map(helloworld_msgpack, helloworld_msgpack + helloworld_msgpack_len);
  }
}
