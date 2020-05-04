#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

#include "msgpack.h"

namespace {
[[noreturn]] void internal_error() {
  printf("internal error\n");
  exit(1);
}
} // namespace

namespace msgpack {

typedef enum : uint8_t {
#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER) NAME,
#include "msgpack.def"
#undef X
} type;

typedef enum : uint8_t {
  boolean,
  unsigned_integer,
  signed_integer,
  string,
  array,
  map,
  other,
} coarse_type;

constexpr bool is_boolean(type ty) { return ty == t || ty == f; }
constexpr bool is_unsigned_integer(type ty) {
  return ty == posfixint || ty == uint8 || ty == uint16 || ty == uint32 ||
         ty == uint64;
}
constexpr bool is_signed_integer(type ty) {
  return ty == negfixint || ty == int8 || ty == int16 || ty == int32 ||
         ty == int64;
}
constexpr bool is_string(type ty) {
  return ty == fixstr || ty == str8 || ty == str16 || ty == str32;
}
constexpr bool is_array(type ty) {
  return ty == fixarray || ty == array16 || ty == array32;
}
constexpr bool is_map(type ty) {
  return ty == fixmap || ty == map16 || ty == map32;
}
constexpr coarse_type categorize(type ty) {
  // TODO: Change to switch when C++14 can be assumed
  return is_boolean(ty)
             ? boolean
             : is_unsigned_integer(ty)
                   ? unsigned_integer
                   : is_signed_integer(ty)
                         ? signed_integer
                         : is_string(ty)
                               ? string
                               : is_array(ty) ? array
                                              : is_map(ty) ? map : other;
}

const char *type_name(type ty) {
  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER)                                  \
  case NAME:                                                                   \
    return #NAME;
#include "msgpack.def"
#undef X
  }
  internal_error();
}

unsigned bytes_used_fixed(msgpack::type ty) {
  using namespace msgpack;
  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER)                                  \
  case NAME:                                                                   \
    return WIDTH;
#include "msgpack.def"
#undef X
  }
  internal_error();
}

static msgpack::type parse_type1(unsigned char x) {

#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER)                                  \
  if (x >= LOWER && x <= UPPER) {                                              \
    return NAME;                                                               \
  } else
#include "msgpack.def"
#undef X
  { internal_error(); }
}

static msgpack::type parse_type2(unsigned char x) {
  static const msgpack::type tab[256] = {
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  posfixint, posfixint, posfixint, posfixint,
      posfixint, posfixint,  fixmap,    fixmap,    fixmap,    fixmap,
      fixmap,    fixmap,     fixmap,    fixmap,    fixmap,    fixmap,
      fixmap,    fixmap,     fixmap,    fixmap,    fixmap,    fixmap,
      fixarray,  fixarray,   fixarray,  fixarray,  fixarray,  fixarray,
      fixarray,  fixarray,   fixarray,  fixarray,  fixarray,  fixarray,
      fixarray,  fixarray,   fixarray,  fixarray,  fixstr,    fixstr,
      fixstr,    fixstr,     fixstr,    fixstr,    fixstr,    fixstr,
      fixstr,    fixstr,     fixstr,    fixstr,    fixstr,    fixstr,
      fixstr,    fixstr,     fixstr,    fixstr,    fixstr,    fixstr,
      fixstr,    fixstr,     fixstr,    fixstr,    fixstr,    fixstr,
      fixstr,    fixstr,     fixstr,    fixstr,    fixstr,    fixstr,
      nil,       never_used, f,         t,         bin8,      bin16,
      bin32,     ext8,       ext16,     ext32,     float32,   float64,
      uint8,     uint16,     uint32,    uint64,    int8,      int16,
      int32,     int64,      fixext1,   fixext2,   fixext4,   fixext8,
      fixext16,  str8,       str16,     str32,     array16,   array32,
      map16,     map32,      negfixint, negfixint, negfixint, negfixint,
      negfixint, negfixint,  negfixint, negfixint, negfixint, negfixint,
      negfixint, negfixint,  negfixint, negfixint, negfixint, negfixint,
      negfixint, negfixint,  negfixint, negfixint, negfixint, negfixint,
      negfixint, negfixint,  negfixint, negfixint, negfixint, negfixint,
      negfixint, negfixint,  negfixint, negfixint,
  };
  return tab[x];
}

msgpack::type parse_type(unsigned char x) {
  const bool tab = false;
  // CFA is failing to eliminate the trailing internal_error call
  return tab ? parse_type2(x) : parse_type1(x);
}

extern "C" void gen_parse_type_tab(void) {
  printf("msgpack::type tab[256] = {\n");
  for (unsigned i = 0; i < 256; i++) {
    printf("  /*[%3u]*/ %s,\n", i, type_name(parse_type1((unsigned char)i)));
  }
  printf("}\n");
}

} // namespace msgpack

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

typedef uint64_t (*payload_info_t)(const unsigned char *);

namespace {
namespace payload {
uint64_t read_zero(const unsigned char *) { return 0; }

// Read the first byte and zero/sign extend it
uint64_t read_embedded_u8(const unsigned char *start) { return start[0]; }
uint64_t read_embedded_s8(const unsigned char *start) {
  int64_t res = bitcast<uint8_t, int8_t>(start[0]);
  return bitcast<int64_t, uint64_t>(res);
}

// Read a masked part of the first byte
uint64_t read_via_mask_0x1(const unsigned char *start) { return *start & 0x1u; }
uint64_t read_via_mask_0xf(const unsigned char *start) { return *start & 0xfu; }
uint64_t read_via_mask_0x1f(const unsigned char *start) {
  return *start & 0x1fu;
}

// Read 1/2/4/8 bytes immediately following the type byte and zero/sign extend
// Big endian format.
uint64_t read_size_field_u8(const unsigned char *from) {
  from++;
  return from[0];
}

// TODO: detect whether host is little endian or not, and whether the intrinsic
// is available. And probably use the builtin to test the diy
const bool use_bswap = true;

uint64_t read_size_field_u16(const unsigned char *from) {
  from++;
  if (use_bswap) {
    uint16_t b;
    memcpy(&b, from, 2);
    return __builtin_bswap16(b);
  } else {
    return (from[0] << 8u) | from[1];
  }
}
uint64_t read_size_field_u32(const unsigned char *from) {
  from++;
  if (use_bswap) {
    uint32_t b;
    memcpy(&b, from, 4);
    return __builtin_bswap32(b);
  } else {
    return (from[0] << 24u) | (from[1] << 16u) | (from[2] << 8u) |
           (from[3] << 0u);
  }
}
uint64_t read_size_field_u64(const unsigned char *from) {
  from++;
  if (use_bswap) {
    uint64_t b;
    memcpy(&b, from, 8);
    return __builtin_bswap64(b);
  } else {
    return ((uint64_t)from[0] << 56u) | ((uint64_t)from[1] << 48u) |
           ((uint64_t)from[2] << 40u) | ((uint64_t)from[3] << 32u) |
           (from[4] << 24u) | (from[5] << 16u) | (from[6] << 8u) |
           (from[7] << 0u);
  }
}

uint64_t read_size_field_s8(const unsigned char *from) {
  uint8_t u = read_size_field_u8(from);
  int64_t res = bitcast<uint8_t, int8_t>(u);
  return bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s16(const unsigned char *from) {
  uint16_t u = read_size_field_u16(from);
  int64_t res = bitcast<uint16_t, int16_t>(u);
  return bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s32(const unsigned char *from) {
  uint32_t u = read_size_field_u32(from);
  int64_t res = bitcast<uint32_t, int32_t>(u);
  return bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s64(const unsigned char *from) {
  uint64_t u = read_size_field_u64(from);
  int64_t res = bitcast<uint64_t, int64_t>(u);
  return bitcast<int64_t, uint64_t>(res);
}
} // namespace payload
} // namespace

payload_info_t payload_info(msgpack::type ty) {
  using namespace msgpack;
  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER)                                  \
  case NAME:                                                                   \
    return payload::PAYLOAD;
#include "msgpack.def"
#undef X
  }
  internal_error();
}

// Only failure mode is going to be out of bounds
// Return NULL on out of bounds, otherwise start of the next entry

namespace fallback {

void nop_string(size_t, const unsigned char *) {}
void nop_signed(int64_t) {}
void nop_unsigned(uint64_t) {}
void nop_boolean(bool) {}
void nop_array_elements(byte_range) {}
void nop_map_elements(byte_range, byte_range) {}

const unsigned char *array(uint64_t N, byte_range bytes,
                           std::function<void(byte_range)> callback) {
  for (uint64_t i = 0; i < N; i++) {
    const unsigned char *next = skip_next_message(bytes.start, bytes.end);
    if (!next) {
      return 0;
    }
    callback(bytes);

    bytes.start = next;
  }
  return bytes.start;
}

const unsigned char *map(uint64_t N, byte_range bytes,
                         std::function<void(byte_range, byte_range)> callback) {

  for (uint64_t i = 0; i < N; i++) {
    const unsigned char *start_key = bytes.start;
    const unsigned char *end_key = skip_next_message(start_key, bytes.end);

    if (!end_key) {
      break;
    }

    const unsigned char *start_value = end_key;
    const unsigned char *end_value = skip_next_message(start_value, bytes.end);

    if (!end_value) {
      break;
    }

    callback({start_key, end_key}, {start_value, end_value});

    bytes.start = end_value;
  }
  return bytes.start;
}

const unsigned char *nop_map(uint64_t N, byte_range bytes) {
  return map(N, bytes, nop_map_elements);
}

const unsigned char *nop_array(uint64_t N, byte_range bytes) {
  return array(N, bytes, nop_array_elements);
}

} // namespace fallback

const unsigned char *fallback::skip_next_message(const unsigned char *start,
                                                 const unsigned char *end) {
  return handle_msgpack({start, end}, functors());
}

const unsigned char *
fallback::skip_next_message_templated(const unsigned char *start,
                                      const unsigned char *end) {
  return handle_msgpack({start, end}, functors_nop());
}

namespace {

template <msgpack::type ty, typename F>
const unsigned char *handle_msgpack_given_type(byte_range bytes, F f) {
  const unsigned char *start = bytes.start;
  const unsigned char *end = bytes.end;
  const uint64_t available = end - start;
  assert(available != 0);

  // Would be better to skip the bytes used calculation when the result value is
  // not used and the type has no handler registered
  const uint64_t bytes_used = bytes_used_fixed(ty);
  if (available < bytes_used) {
    return 0;
  }
  const uint64_t available_post_header = available - bytes_used;

  // If this is called with a compile time constant ty, can inline the
  // associated function Means duplication in the following switch
  const payload_info_t info = payload_info(ty);
  const uint64_t N = info(start);

  constexpr msgpack::coarse_type cty = categorize(ty);

  switch (cty) {
  case msgpack::boolean: {
    // t is 0b11000010, f is 0b11000011, masked with 0x1
    f.cb_boolean(N);
    return start + bytes_used;
  }

  case msgpack::unsigned_integer: {
    f.cb_unsigned(N);
    return start + bytes_used;
  }

  case msgpack::signed_integer: {
    f.cb_signed(bitcast<uint64_t, int64_t>(N));
    return start + bytes_used;
  }

  case msgpack::string: {
    if (available_post_header < N) {
      return 0;
    } else {
      f.cb_string(N, start + bytes_used);
      return start + bytes_used + N;
    }
  }

  case msgpack::array: {
    return f.cb_array(N, byte_range{start + bytes_used, end});
  }

  case msgpack::map: {
    return f.cb_map(N, {start + bytes_used, end});
  }

  case msgpack::other: {
    if (available_post_header < N) {
      return 0;
    }
    return start + bytes_used + N;
  }
  }
  internal_error();
}
} // namespace

template <typename F>
const unsigned char *handle_msgpack(byte_range bytes, F f) {

  const unsigned char *start = bytes.start;
  const unsigned char *end = bytes.end;
  const uint64_t available = end - start;
  if (available == 0) {
    return 0;
  }
  const msgpack::type ty = msgpack::parse_type(*start);
  const bool asm_markers = true;

  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER)                                  \
  case msgpack::NAME: {                                                        \
    if (asm_markers)                                                           \
      asm("# Handle msgpack::" #NAME " begin");                                \
    const unsigned char *res =                                                 \
        handle_msgpack_given_type<msgpack::NAME, F>(bytes, f);                 \
    if (asm_markers)                                                           \
      asm("# Handle msgpack::" #NAME " finish");                               \
    return res;                                                                \
  }

#include "msgpack.def"
#undef X
  }

  internal_error();
}

namespace {

template <msgpack::coarse_type C>
bool message_is_coarse_type(byte_range bytes) {
  const uint64_t available = bytes.end - bytes.start;
  // Empty range, contains no typed message
  if (available == 0) {
    return msgpack::other;
  }

  const msgpack::type ty = msgpack::parse_type(*bytes.start);
  msgpack::coarse_type cty = categorize(ty);
  if (cty != C) {
    return false;
  }

  // truncated header
  const uint64_t bytes_used = bytes_used_fixed(ty);
  return available >= bytes_used;
}
} // namespace

bool is_boolean(byte_range bytes) {
  return message_is_coarse_type<msgpack::boolean>(bytes);
}
bool is_unsigned(byte_range bytes) {
  return message_is_coarse_type<msgpack::unsigned_integer>(bytes);
}
bool is_signed(byte_range bytes) {
  return message_is_coarse_type<msgpack::signed_integer>(bytes);
}
bool is_string(byte_range bytes) {
  return message_is_coarse_type<msgpack::string>(bytes);
}
bool is_array(byte_range bytes) {
  return message_is_coarse_type<msgpack::array>(bytes);
}
bool is_map(byte_range bytes) {
  return message_is_coarse_type<msgpack::map>(bytes);
}

bool message_is_string(byte_range bytes, const char *needle) {
  bool matched = false;
  functors f;
  size_t needleN = strlen(needle);

  f.cb_string = [=, &matched](size_t N, const unsigned char *str) {
    if (N == needleN) {
      if (memcmp(needle, str, N) == 0) {
        matched = true;
      }
    }
  };

  handle_msgpack(bytes, f);
  return matched;
}

template const unsigned char *handle_msgpack(byte_range, functors);
template const unsigned char *handle_msgpack(byte_range, functors_nop);
template const unsigned char *
    handle_msgpack(byte_range, only_apply_if_top_level_is_unsigned);
template const unsigned char *handle_msgpack(byte_range,
                                             functors_ignore_nested);

template const unsigned char *handle_msgpack(byte_range, example);

void foreach_map(byte_range bytes,
                 std::function<void(byte_range, byte_range)> callback) {
  functors f;
  f.cb_map_elements = callback;
  handle_msgpack(bytes, f);
}

void foreach_array(byte_range bytes, std::function<void(byte_range)> callback) {
  functors f;
  f.cb_array_elements = callback;
  handle_msgpack(bytes, f);
}

void msgpack::dump(byte_range bytes) {
  functors f;
  unsigned indent = 0;
  const unsigned by = 2;

  f.cb_string = [&](size_t N, const unsigned char *bytes) {
    char *tmp = (char *)malloc(N + 1);
    memcpy(tmp, bytes, N);
    tmp[N] = '\0';
    printf("\"%s\"", tmp);
    free(tmp);
  };

  f.cb_signed = [&](int64_t x) { printf("%ld", x); };
  f.cb_unsigned = [&](uint64_t x) { printf("%lu", x); };

  f.cb_array = [&](uint64_t N, byte_range bytes) -> const unsigned char * {
    printf("\n%*s[\n", indent, "");
    indent += by;

    for (uint64_t i = 0; i < N; i++) {
      indent += by;
      printf("%*s", indent, "");
      const unsigned char *next = handle_msgpack(bytes, f);
      printf(",\n");
      indent -= by;
      bytes.start = next;
      if (!next) {
        break;
      }
    }
    indent -= by;
    printf("%*s]", indent, "");

    return bytes.start;
  };

  f.cb_map = [&](uint64_t N, byte_range bytes) -> const unsigned char * {
    printf("\n%*s{\n", indent, "");
    indent += by;

    for (uint64_t i = 0; i < 2 * N; i += 2) {
      const unsigned char *start_key = bytes.start;
      printf("%*s", indent, "");
      const unsigned char *end_key = handle_msgpack({start_key, bytes.end}, f);
      if (!end_key) {
        break;
      }

      printf(" : ");

      const unsigned char *start_value = end_key;
      const unsigned char *end_value =
          handle_msgpack({start_value, bytes.end}, f);

      if (!end_value) {
        break;
      }

      printf(",\n");

      bytes.start = end_value;
    }

    indent -= by;
    printf("%*s}", indent, "");

    return bytes.start;
  };

  handle_msgpack(bytes, f);
  printf("\n");
}
