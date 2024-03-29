#include "msgpack.h"

#include <cstdint>
#include <cstring>
#include <cstdlib>

namespace msgpack {
const char *type_name(type ty) {
  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER)                                  \
  case NAME:                                                                   \
    return #NAME;
#include "msgpack.def"
#undef X
  }
  __builtin_unreachable();
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
  __builtin_unreachable();
}

static msgpack::type parse_type1(unsigned char x) {

#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER)                                  \
  if (x >= LOWER && x <= UPPER) {                                              \
    return NAME;                                                               \
  } else
#include "msgpack.def"
#undef X
  {   __builtin_unreachable(); }
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
  return tab ? parse_type2(x) : parse_type1(x);
}

extern "C" void gen_parse_type_tab(void) {
  printf("msgpack::type tab[256] = {\n");
  for (unsigned i = 0; i < 256; i++) {
    printf("  /*[%3u]*/ %s,\n", i, type_name(parse_type1((unsigned char)i)));
  }
  printf("}\n");
}

template <typename T, typename R> R bitcast(T x) {
  static_assert(sizeof(T) == sizeof(R), "");
  R tmp;
  memcpy(&tmp, &x, sizeof(T));
  return tmp;
}
template int64_t bitcast<uint64_t, int64_t>(uint64_t);
} // namespace msgpack

// Helper functions for reading additional payload from the header
// Depending on the type, this can be a number of bytes, elements,
// key-value pairs or an embedded integer.
// Each takes a pointer to the start of the header and returns a uint64_t

namespace {
namespace payload {
uint64_t read_zero(const unsigned char *) { return 0; }

// Read the first byte and zero/sign extend it
uint64_t read_embedded_u8(const unsigned char *start) { return start[0]; }
uint64_t read_embedded_s8(const unsigned char *start) {
  int64_t res = msgpack::bitcast<uint8_t, int8_t>(start[0]);
  return msgpack::bitcast<int64_t, uint64_t>(res);
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
    return ((uint64_t)from[0] << 56u) |
           ((uint64_t)from[1] << 48u) |
           ((uint64_t)from[2] << 40u) |
           ((uint64_t)from[3] << 32u) |
           (from[4] << 24u) |
           (from[5] << 16u) |
           (from[6] << 8u) |
           (from[7] << 0u);
  }
}

uint64_t read_size_field_s8(const unsigned char *from) {
  uint8_t u = read_size_field_u8(from);
  int64_t res = msgpack::bitcast<uint8_t, int8_t>(u);
  return msgpack::bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s16(const unsigned char *from) {
  uint16_t u = read_size_field_u16(from);
  int64_t res = msgpack::bitcast<uint16_t, int16_t>(u);
  return msgpack::bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s32(const unsigned char *from) {
  uint32_t u = read_size_field_u32(from);
  int64_t res = msgpack::bitcast<uint32_t, int32_t>(u);
  return msgpack::bitcast<int64_t, uint64_t>(res);
}
uint64_t read_size_field_s64(const unsigned char *from) {
  uint64_t u = read_size_field_u64(from);
  int64_t res = msgpack::bitcast<uint64_t, int64_t>(u);
  return msgpack::bitcast<int64_t, uint64_t>(res);
}
} // namespace payload
} // namespace

namespace msgpack {

payload_info_t payload_info(msgpack::type ty) {
  using namespace msgpack;
  switch (ty) {
#define X(NAME, WIDTH, PAYLOAD, LOWER, UPPER)                                  \
  case NAME:                                                                   \
    return payload::PAYLOAD;
#include "msgpack.def"
#undef X
  }
  __builtin_unreachable();
}

struct functors_nop : public functors_defaults<functors_nop> {
  static_assert(has_default_string() == true, "");
  static_assert(has_default_boolean() == true, "");
  static_assert(has_default_signed() == true, "");
  static_assert(has_default_unsigned() == true, "");
  static_assert(has_default_array_elements() == true, "");
  static_assert(has_default_map_elements() == true, "");
  static_assert(has_default_array() == true, "");
  static_assert(has_default_map() == true, "");
};

const unsigned char *fallback::skip_next_message(const unsigned char *start,
                                                 const unsigned char *end) {

  const unsigned char * good = handle_msgpack({start, end}, functors_nop());
  assert(good == fallback::skip_number_contiguous_messages(1, start, end));
  return good;
}

struct functors_message_skip : public functors_defaults<functors_message_skip> {
  functors_message_skip(uint64_t & n) : number_remaining(n) {}
  
  uint64_t & number_remaining;

  // On an array or map, increment the number of messages to skip over and
  // return a pointer to the first such message

  const unsigned char *handle_array(uint64_t N, byte_range bytes) {
    // An array is N contiguous messages
    static_assert(has_default_array_elements() == true, "");
    number_remaining += N;
    return bytes.start;
  }

  const unsigned char *handle_map(uint64_t N, byte_range bytes) {
    // A map is 2*N contiguous messages
    static_assert(has_default_map_elements() == true, "");
    number_remaining += 2*N;
    return bytes.start;
  }
};

const unsigned char *fallback::skip_number_contiguous_messages(
    uint64_t N, const unsigned char *start, const unsigned char *end) {

  uint64_t number_remaining = N;
  auto skip = functors_message_skip(number_remaining);

  while (number_remaining != 0) {
    const unsigned char *r = handle_msgpack({start, end}, skip);
    number_remaining--;
    if (!r) {
      return nullptr;
    }
    start = r;
  }

  return start;
}

} // namespace msgpack

namespace {
template <msgpack::coarse_type C>
bool message_is_coarse_type(msgpack::byte_range bytes) {
  const uint64_t available = bytes.end - bytes.start;
  // Empty range, contains no typed message
  if (available == 0) {
    return false;
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

namespace msgpack {
bool message_is_string(byte_range bytes, const char *needle) {
  bool matched = false;
  size_t needleN = strlen(needle);

  foronly_string(bytes, [=, &matched](size_t N, const unsigned char *str) {
    if (N == needleN) {
      if (memcmp(needle, str, N) == 0) {
        matched = true;
      }
    }
  });
  return matched;
}

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

void dump(byte_range bytes) {
  struct inner : functors_defaults<inner> {
    inner(unsigned indent) : indent(indent) {}
    const unsigned by = 2;
    unsigned indent = 0;

    void handle_string(size_t N, const unsigned char *bytes) {
      char *tmp = (char *)malloc(N + 1);
      memcpy(tmp, bytes, N);
      tmp[N] = '\0';
      printf("\"%s\"", tmp);
      free(tmp);
    }

    void handle_signed(int64_t x) { printf("%ld", x); }
    void handle_unsigned(uint64_t x) { printf("%lu", x); }

    const unsigned char *handle_array(uint64_t N, byte_range bytes) {
      printf("\n%*s[\n", indent, "");
      indent += by;

      for (uint64_t i = 0; i < N; i++) {
        indent += by;
        printf("%*s", indent, "");
        const unsigned char *next = handle_msgpack<inner>(bytes, {indent});
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
    }

    const unsigned char *handle_map(uint64_t N, byte_range bytes) {
      printf("\n%*s{\n", indent, "");
      indent += by;

      for (uint64_t i = 0; i < 2 * N; i += 2) {
        const unsigned char *start_key = bytes.start;
        printf("%*s", indent, "");
        const unsigned char *end_key =
            handle_msgpack<inner>({start_key, bytes.end}, {indent});
        if (!end_key) {
          break;
        }

        printf(" : ");

        const unsigned char *start_value = end_key;
        const unsigned char *end_value =
            handle_msgpack<inner>({start_value, bytes.end}, {indent});

        if (!end_value) {
          break;
        }

        printf(",\n");
        bytes.start = end_value;
      }

      indent -= by;
      printf("%*s}", indent, "");

      return bytes.start;
    }
  };

  handle_msgpack<inner>(bytes, {0});
  printf("\n");
}

} // namespace msgpack
