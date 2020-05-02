#ifndef MSGPACK_H
#define MSGPACK_H

#include <functional>

#include <cstring> // memcpy

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
  void handle_string(size_t N, const unsigned char *str) {
    if (verbose) {
      char *tmp = (char *)malloc(N + 1);
      memcpy(tmp, str, N);
      tmp[N] = '\n';
      printf("Default handle string %s\n", tmp);
      free(tmp);
    }
    fallback::nop_string(N, str);
  }
  void handle_boolean(bool x) {
    if (verbose) {
      printf("Default handle bool %u\n", x);
    }
    fallback::nop_boolean(x);
  }
  void handle_signed(int64_t x) {
    if (verbose) {
      printf("Default handle unsigned %ld\n", x);
    }
    fallback::nop_signed(x);
  }
  void handle_unsigned(uint64_t x) {
    if (verbose) {
      printf("Default handle unsigned %lu\n", x);
    }
    fallback::nop_unsigned(x);
  }

  void handle_map_elements(byte_range key, byte_range value) {
    if (verbose) {
      printf("Default map elements, ignore key/value\n");
    }
    fallback::nop_map_elements(key, value);
  }

  void handle_array_elements(byte_range bytes) {
    if (verbose) {
      printf("Default array elements, ignore bytes\n");
    }
    fallback::nop_array_elements(bytes);
  }

  const unsigned char *handle_array(uint64_t N, byte_range bytes) {
    if (verbose) {
      printf("Default array, N = %lu\n", N);
    }

    for (uint64_t i = 0; i < N; i++) {
      const unsigned char *next =
          fallback::skip_next_message_templated(bytes.start, bytes.end);
      if (!next) {
        return 0;
      }

      derived().handle_array_elements(bytes);

      bytes.start = next;
    }
    return bytes.start;
  }

  const unsigned char *handle_map(uint64_t N, byte_range bytes) {
    if (verbose) {
      printf("Default map, N = %lu\n", N);
    }
    for (uint64_t i = 0; i < N; i++) {
      const unsigned char *start_key = bytes.start;
      const unsigned char *end_key =
          fallback::skip_next_message_templated(start_key, bytes.end);

      if (!end_key) {
        break;
      }

      const unsigned char *start_value = end_key;
      const unsigned char *end_value =
          fallback::skip_next_message_templated(start_value, bytes.end);

      if (!end_value) {
        break;
      }
      derived().handle_map_elements({start_key, end_key},
                                    {start_value, end_value});

      bytes.start = end_value;
    }
    return bytes.start;
  }
};

struct functors_nop : public functors_defaults<functors_nop> {};

template <typename F> const unsigned char *handle_msgpack(byte_range, F f);

bool message_is_string(byte_range bytes, const char *str);

void foreach_map(byte_range,
                 std::function<void(byte_range, byte_range)> callback);

void foreach_array(byte_range, std::function<void(byte_range)> callback);

namespace msgpack {
// Crude approximation to json
void dump(byte_range);
} // namespace msgpack

#endif
