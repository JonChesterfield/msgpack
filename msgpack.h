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

const unsigned char *skip_next_message_templated(const unsigned char *start,
                                                 const unsigned char *end);

void nop_string(size_t, const unsigned char *);
void nop_signed(int64_t);
void nop_unsigned(uint64_t);
void nop_boolean(bool);
void nop_array_elements(byte_range);
void nop_map_elements(byte_range, byte_range);

const unsigned char *nop_map(uint64_t N, byte_range);
const unsigned char *nop_array(uint64_t N, byte_range);

const unsigned char *array(uint64_t N, byte_range,
                           std::function<void(byte_range)> callback);
const unsigned char *map(uint64_t N, byte_range,
                         std::function<void(byte_range, byte_range)> callback);
} // namespace fallback

struct functors {

  std::function<void(size_t, const unsigned char *)> cb_string =
      fallback::nop_string;

  std::function<void(int64_t)> cb_signed = fallback::nop_signed;

  std::function<void(uint64_t)> cb_unsigned = fallback::nop_unsigned;

  std::function<void(bool)> cb_boolean = fallback::nop_boolean;

  std::function<void(byte_range, byte_range)> cb_map_elements =
      fallback::nop_map_elements;

  std::function<void(byte_range)> cb_array_elements =
      fallback::nop_array_elements;

  std::function<const unsigned char *(uint64_t N, byte_range)> cb_array =
      [=](uint64_t N, byte_range bytes) {
        return fallback::array(N, bytes, this->cb_array_elements);
      };

  std::function<const unsigned char *(uint64_t N, byte_range)> cb_map =
      [=](uint64_t N, byte_range bytes) {
        return fallback::map(N, bytes, this->cb_map_elements);
      };

  // std::function implementation can't answer these efficiently
  static constexpr bool has_default_boolean() { return false; }
  static constexpr bool has_default_unsigned() { return false; }
  static constexpr bool has_default_signed() { return false; }
  static constexpr bool has_default_string() { return false; }
  static constexpr bool has_default_array() { return false; }
  static constexpr bool has_default_array_elements() { return false; }
  static constexpr bool has_default_map() { return false; }
  static constexpr bool has_default_map_elements() { return false; }
};

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
          fallback::skip_next_message_templated(bytes.start, bytes.end);
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
          fallback::skip_next_message_templated(start_key, bytes.end);

      if (!end_key) {
        return nullptr;
      }

      const unsigned char *start_value = end_key;
      const unsigned char *end_value =
          fallback::skip_next_message_templated(start_value, bytes.end);

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

struct functors_ignore_nested
    : public functors_defaults<functors_ignore_nested> {
  const unsigned char *handle_array(uint64_t N, byte_range bytes) {
    return bytes.end;
  }
  const unsigned char *handle_map(uint64_t N, byte_range bytes) {
    return bytes.end;
  }

  static_assert(has_default_array() == false, "");
  static_assert(has_default_map() == false, "");
  static_assert(has_default_map_elements() == true, "");
};

struct only_apply_if_top_level_is_unsigned
    : public functors_defaults<only_apply_if_top_level_is_unsigned> {
  only_apply_if_top_level_is_unsigned(void (*f)(uint64_t, void *), void *st)
      : f(f), st(st) {}

  void handle_unsigned(uint64_t x) { f(x, st); }

private:
  void (*f)(uint64_t, void *);
  void *st;
};

struct example : public functors_defaults<example> {
  example() { static_assert(has_default_string() == false, ""); }

  // Possible bug here. Adding this == true before the call
  // makes both the others fail
  // static_assert(has_default_string() == true, "");
  void handle_string(size_t N, const unsigned char *str) {
    printf("Called derived handle string\n");
  }
  static_assert(has_default_string() == false, "");

  // Here it just fails, as it should
  //   static_assert(handle_string_is_default() == true, "");
  void handle_array_elements(byte_range bytes) {
    printf("called derived array elements\n");
  }
};

template <typename F> const unsigned char *handle_msgpack(byte_range, F f);
template <typename F> void handle_msgpack_void(byte_range, F f);

bool message_is_string(byte_range bytes, const char *str);

void foreach_map(byte_range,
                 std::function<void(byte_range, byte_range)> callback);

void foreach_array(byte_range, std::function<void(byte_range)> callback);

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
