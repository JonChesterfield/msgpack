#ifndef MSGPACK_H
#define MSGPACK_H

#include <cstring> // memcpy
#include <functional>

namespace msgpack {

// The message pack format is dynamically typed, schema-less. Format is:
// message: [type][header][payload]
// where type is one byte, header length is a fixed length function of type
// payload is zero to N bytes, with the length encoded in [type][header]

// Scalar fields include boolean, signed integer, float, string etc
// Composite types are sequences of messages
// Array field is [header][element][element]...
// Map field is [header][key][value][key][value]...

// Multibyte integer fields are big endian encoded
// The map key can be any message type
// Maps may contain duplicate keys
// Data is not uniquely encoded, e.g. integer "8" may be stored as one byte or
// in as many as nine, as signed or unsigned. Implementation defined.
// Similarly "foo" may embed the length in the type field or in multiple bytes

// This parser is structured as an iterator over a sequence of bytes.
// It calls a user provided function on each message in order to extract fields
// The default implementation for each scalar type is to do nothing. For map or
// arrays, the default implementation returns just after that message to support
// iterating to the next message, but otherwise has no effect.

struct byte_range {
  const unsigned char *start;
  const unsigned char *end;
};

namespace fallback {

const unsigned char *skip_next_message(const unsigned char *start,
                                       const unsigned char *end);

const unsigned char *skip_number_contiguous_messages(uint64_t N,
                                                     const unsigned char *start,
                                                     const unsigned char *end);

} // namespace fallback

template <typename Derived> class functors_defaults {
public:
  void cb_string(size_t N, const unsigned char *str) {
    derived().handle_string(N, str);
  }

  void cb_boolean(bool x) { derived().handle_boolean(x); }

  void cb_signed(int64_t x) { derived().handle_signed(x); }

  void cb_unsigned(uint64_t x) { derived().handle_unsigned(x); }

  void cb_array_elements(byte_range bytes) {
    derived().handle_array_elements(bytes);
  }

  void cb_map_elements(byte_range key, byte_range value) {
    derived().handle_map_elements(key, value);
  }

  const unsigned char *cb_array(uint64_t N, byte_range bytes) {
    return derived().handle_array(N, bytes);
  }

  const unsigned char *cb_map(uint64_t N, byte_range bytes) {
    return derived().handle_map(N, bytes);
  }

private:
  Derived &derived() { return *static_cast<Derived *>(this); }

  static const bool verbose = false;
  // Default implementations
  void handle_string(size_t, const unsigned char *) {}
  void handle_boolean(bool) {}
  void handle_signed(int64_t) {}
  void handle_unsigned(uint64_t) {}
  void handle_map_elements(byte_range, byte_range) {}
  void handle_array_elements(byte_range) {}

  const unsigned char *handle_array(uint64_t N, byte_range bytes) {
    for (uint64_t i = 0; i < N; i++) {
      const unsigned char *next =
          fallback::skip_next_message(bytes.start, bytes.end);
      if (!next) {
        return nullptr;
      }

      cb_array_elements(bytes);

      bytes.start = next;
    }
    return bytes.start;
  }

  const unsigned char *handle_map(uint64_t N, byte_range bytes) {
    for (uint64_t i = 0; i < N; i++) {
      const unsigned char *start_key = bytes.start;
      const unsigned char *end_key =
          fallback::skip_next_message(start_key, bytes.end);

      if (!end_key) {
        return nullptr;
      }

      const unsigned char *start_value = end_key;
      const unsigned char *end_value =
          fallback::skip_next_message(start_value, bytes.end);

      if (!end_value) {
        return nullptr;
      }
      cb_map_elements({start_key, end_key}, {start_value, end_value});

      bytes.start = end_value;
    }
    return bytes.start;
  }

public:
  constexpr static bool has_default_string() {
    return &functors_defaults::handle_string == &Derived::handle_string;
  }
  constexpr static bool has_default_boolean() {
    return &functors_defaults::handle_boolean == &Derived::handle_boolean;
  }
  constexpr static bool has_default_signed() {
    return &functors_defaults::handle_signed == &Derived::handle_signed;
  }
  constexpr static bool has_default_unsigned() {
    return &functors_defaults::handle_unsigned == &Derived::handle_unsigned;
  }
  constexpr static bool has_default_array_elements() {
    return &functors_defaults::handle_array_elements ==
           &Derived::handle_array_elements;
  }
  constexpr static bool has_default_map_elements() {
    return &functors_defaults::handle_map_elements ==
           &Derived::handle_map_elements;
  }
  constexpr static bool has_default_array() {
    return &functors_defaults::handle_array == &Derived::handle_array;
  }
  constexpr static bool has_default_map() {
    return &functors_defaults::handle_map == &Derived::handle_map;
  }
};

struct example : public functors_defaults<example> {
  example() { static_assert(has_default_string() == false, ""); }

  // Possible bug here. Adding this == true before the call
  // makes both the others fail
  // static_assert(has_default_string() == true, "");
  void handle_string(size_t , const unsigned char *) {
    printf("Called derived handle string\n");
  }
  static_assert(has_default_string() == false, "");

  // Here it just fails, as it should
  //   static_assert(handle_string_is_default() == true, "");
  void handle_array_elements(byte_range) {
    printf("called derived array elements\n");
  }
};

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

[[noreturn]] void internal_error();
type parse_type(unsigned char x);
unsigned bytes_used_fixed(type ty);

typedef uint64_t (*payload_info_t)(const unsigned char *);
payload_info_t payload_info(msgpack::type ty);
template <typename T, typename R> R bitcast(T x);

namespace {

template <bool ResUsed, msgpack::coarse_type cty, typename F>
constexpr bool can_early_return() {
  // If the returned pointer is unused and the function called is known to
  // do nothing other than compute the return pointer, it doesn't need to
  // be called. The CFA needed to prove this in the compiler doesn't appear
  // to be sufficient yet, so hardcode the implementation detail that the
  // defaults to nothing other than compute the return pointer
  // TODO: Rename parts of this mechanism
  return ResUsed ? false
                 : cty == msgpack::boolean
                       ? F::has_default_boolean()
                       : cty == msgpack::unsigned_integer
                             ? F::has_default_unsigned()
                             : cty == msgpack::signed_integer
                                   ? F::has_default_signed()
                                   : cty == msgpack::string
                                         ? F::has_default_string()
                                         : cty == msgpack::array
                                               ? F::has_default_array()
                                               : cty == msgpack::map
                                                     ? F::has_default_map()
                                                     : cty == msgpack::other
                                                           ? true
                                                           : false;
}
} // namespace

namespace cat {
constexpr bool is_boolean(type ty) { return (ty == t || ty == f); }
constexpr bool is_unsigned_integer(type ty) {
  return (ty == posfixint || ty == uint8 || ty == uint16 || ty == uint32 ||
          ty == uint64);
}
constexpr bool is_signed_integer(type ty) {
  return (ty == negfixint || ty == int8 || ty == int16 || ty == int32 ||
          ty == int64);
}
constexpr bool is_string(type ty) {
  return (ty == fixstr || ty == str8 || ty == str16 || ty == str32);
}
constexpr bool is_array(type ty) {
  return (ty == fixarray || ty == array16 || ty == array32);
}
constexpr bool is_map(type ty) {
  return (ty == fixmap || ty == map16 || ty == map32);
}
} // namespace cat
constexpr coarse_type categorize(type ty) {
  // TODO: Change to switch when C++14 can be assumed
  return cat::is_boolean(ty)
             ? boolean
             : cat::is_unsigned_integer(ty)
                   ? unsigned_integer
                   : cat::is_signed_integer(ty)
                         ? signed_integer
                         : cat::is_string(ty)
                               ? string
                               : cat::is_array(ty)
                                     ? array
                                     : cat::is_map(ty) ? map : other;
}

template <bool ResUsed, msgpack::type ty, typename F>
const unsigned char *handle_msgpack_given_type(msgpack::byte_range bytes, F f) {
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

  const payload_info_t info = payload_info(ty);
  const uint64_t N = info(start);

  constexpr msgpack::coarse_type cty = categorize(ty);
  {
    constexpr bool early = can_early_return<ResUsed, cty, F>();
    if (early) {
      return 0;
    }
  }

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
    return f.cb_array(N, {start + bytes_used, end});
  }

  case msgpack::map: {
    return f.cb_map(N, {start + bytes_used, end});
  }

  case msgpack::other: {
    if (!ResUsed) {
      return 0;
    }
    if (available_post_header < N) {
      return 0;
    }
    return start + bytes_used + N;
  }
  }
  internal_error();
}

namespace {
template <bool ResUsed, typename F>
const unsigned char *handle_msgpack_dispatch(msgpack::byte_range bytes, F f) {

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
        handle_msgpack_given_type<ResUsed, msgpack::NAME, F>(bytes, f);        \
    if (asm_markers)                                                           \
      asm("# Handle msgpack::" #NAME " finish");                               \
    return res;                                                                \
  }

#include "msgpack.def"
#undef X
  }

  internal_error();
}
} // namespace

template <typename F>
const unsigned char *handle_msgpack(byte_range bytes, F f) {
  return handle_msgpack_dispatch<true, F>(bytes, f);
}

template <typename F> void handle_msgpack_void(byte_range bytes, F f) {
  handle_msgpack_dispatch<false, F>(bytes, f);
}

bool message_is_string(byte_range bytes, const char *str);

template <typename C> void foronly_string(byte_range bytes, C callback) {
  struct inner : functors_defaults<inner> {
    inner(C &cb) : cb(cb) {}
    C &cb;
    void handle_string(size_t N, const unsigned char *str) { cb(N, str); }
  };
  static_assert(!inner::has_default_string(), "");
  static_assert(inner::has_default_map(), "");
  handle_msgpack_void<inner>(bytes, {callback});
}

template <typename C> void foronly_signed(byte_range bytes, C callback) {
  struct inner : functors_defaults<inner> {
    inner(C &cb) : cb(cb) {}
    C &cb;
    void handle_signed(int64_t x) { cb(x); }
  };

  static_assert(inner::has_default_map() == true, "");
  static_assert(inner::has_default_signed() == false, "");

  handle_msgpack_void<inner>(bytes, {callback});
}

template <typename C> void foronly_unsigned(byte_range bytes, C callback) {
  struct inner : functors_defaults<inner> {
    inner(C &cb) : cb(cb) {}
    C &cb;
    void handle_unsigned(uint64_t x) { cb(x); }
  };

  static_assert(inner::has_default_map() == true, "");
  static_assert(inner::has_default_unsigned() == false, "");

  handle_msgpack_void<inner>(bytes, {callback});
}

  
template <typename C> void foreach_array(byte_range bytes, C callback) {
  struct inner : functors_defaults<inner> {
    inner(C &cb) : cb(cb) {}
    C &cb;
    void handle_array_elements(byte_range element) { cb(element); }
  };
  handle_msgpack_void<inner>(bytes, {callback});
}

template <typename C> void foreach_map(byte_range bytes, C callback) {
  struct inner : functors_defaults<inner> {
    inner(C &cb) : cb(cb) {}
    C &cb;
    void handle_map_elements(byte_range key, byte_range value) {
      cb(key, value);
    }
  };
  handle_msgpack_void<inner>(bytes, {callback});
}

bool is_boolean(byte_range);
bool is_unsigned(byte_range);
bool is_signed(byte_range);
bool is_string(byte_range);
bool is_array(byte_range);
bool is_map(byte_range);

// Crude approximation to json
void dump(byte_range);

} // namespace msgpack

#endif
