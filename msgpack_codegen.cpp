#include "catch.hpp"
#include "msgpack.h"
#include <array>

using namespace msgpack;

namespace detail {

// Plumbing to concatenate std::array at constexpr time
template <size_t... Is> struct seq {};

template <size_t N, size_t... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

template <size_t... Is> struct gen_seq<0, Is...> : seq<Is...> {};

template <size_t N1, size_t... I1, size_t N2, size_t... I2>
constexpr std::array<unsigned char, N1 + N2>
concat(const std::array<unsigned char, N1> &a1,
       const std::array<unsigned char, N2> &a2, seq<I1...>, seq<I2...>) {
  return {a1[I1]..., a2[I2]...};
}

template <size_t N1, size_t N2>
constexpr std::array<unsigned char, N1 + N2>
concat(const std::array<unsigned char, N1> &a1,
       const std::array<unsigned char, N2> &a2) {
  return concat(a1, a2, gen_seq<N1>{}, gen_seq<N2>{});
}

// Convert \0 terminated char string literal to array of unsigned char,
// removing the terminator
template <size_t N, size_t... I>
constexpr std::array<unsigned char, N - 1> str_to_array(const char (&s)[N],
                                                        seq<I...>) {
  return {static_cast<unsigned char>(s[I])...};
}

template <size_t N>
constexpr std::array<unsigned char, N - 1> str_to_array(const char (&s)[N]) {
  return str_to_array(s, gen_seq<N - 1>{});
}

// TODO: Avoid repeating the encodings here
template <size_t N> constexpr std::array<unsigned char, 1> prefix_fixstr() {
  return {{160u + N - 1}};
}

template <size_t N> constexpr std::array<unsigned char, 2> prefix_str8() {
  return {{0xd9, N}};
}

template <size_t N> constexpr std::array<unsigned char, 3> prefix_str16() {
  return {{0xda, 0, N}};
}

template <size_t N> constexpr std::array<unsigned char, 5> prefix_str32() {
  return {{0xdb, 0, 0, 0, N}};
}

template <size_t N> struct matcher {
  // Using a struct to stage the construction, as the string is available at
  // compile time but the byte array is not. This avoids constructing the arrays
  // at runtime (i.e. avoids writing known constants to the stack)

  static_assert(N != 0, "");
  static_assert(N <= 32, "Matcher implemented for strings <= 32 in length");

  constexpr matcher(const char (&str)[N]) : str(str) {}
  const char (&str)[N];

  const std::array<unsigned char, N - 1> arr = str_to_array(str);
  const std::array<unsigned char, N> fixstr = concat(prefix_fixstr<N>(), arr);
  const std::array<unsigned char, N + 1> str8 = concat(prefix_str8<N>(), arr);
  const std::array<unsigned char, N + 2> str16 = concat(prefix_str16<N>(), arr);
  const std::array<unsigned char, N + 4> str32 = concat(prefix_str32<N>(), arr);

  bool operator()(byte_range bytes) const {
    const bool asm_markers = false;
    auto memeq = [](const unsigned char *x, const unsigned char *y,
                    uint64_t sz) -> bool { return memcmp(x, y, sz) == 0; };

    // Need at least enough bytes for the fixstr encoding for any to match
    const uint64_t available = bytes.end - bytes.start;
    if (available < fixstr.size()) {
      return false;
    }

    // Expect the short string to be encoded as a fixstr
    if (asm_markers) {
      asm("# Check fixstr");
    }
    if (memeq(bytes.start, fixstr.data(), fixstr.size())) {
      return true;
    }

    if (available >= str32.size()) {
      if (asm_markers) {
        asm("# Check str32");
      }
      if (memeq(bytes.start, str32.data(), str32.size())) {
        return true;
      }
    }

    if (available >= str16.size()) {
      if (asm_markers) {
        asm("# Check str16");
      }
      if (memeq(bytes.start, str16.data(), str16.size())) {
        return true;
      }
    }

    if (available >= str8.size()) {
      if (asm_markers) {
        asm("# Check str8");
      }
      if (memeq(bytes.start, str8.data(), str8.size())) {
        return true;
      }
    }

    return false;
  }
};

template <size_t N> constexpr matcher<N> make_matcher(const char (&str)[N]) {
  return matcher<N>{str};
}
} // namespace detail

extern "C" bool match_foobar_example(byte_range bytes) {
  return message_is_string(bytes, "badger");
}
extern "C" bool match_badger_example(byte_range bytes) {
  constexpr auto m = detail::make_matcher("badger");
  return m(bytes);
}

extern "C" const unsigned char *
skip_next_message_example(const unsigned char *start,
                          const unsigned char *end) {
  return fallback::skip_next_message(start, end);
}

extern "C" bool message_is_string_example(byte_range bytes, const char *str) {
  return message_is_string(bytes, str);
}

extern "C" void nop_handle_msgpack_example(const unsigned char *start,
                                           const unsigned char *end) {
  struct functors_nop : public functors_defaults<functors_nop> {};
  handle_msgpack_void({start, end}, functors_nop());
}

extern "C" void foronly_unsigned_example(byte_range bytes,
                                         void (*cb)(uint64_t)) {
  foronly_unsigned(bytes, cb);
}

extern "C" void foronly_string_example(byte_range bytes,
                                       void (*cb)(size_t,
                                                  const unsigned char *)) {
  foronly_string(bytes, cb);
}

extern "C" const unsigned char *expt(const unsigned char *start,
                                     const unsigned char *end) {

  example ex;
  ex.cb_string(end - start, start);
  ex.cb_unsigned(4);
  return 0;
}

TEST_CASE(" ex ") {

  SECTION("") {
    const unsigned char *data = (const unsigned char *)"foobar\n";
    expt(data, data + 10);

    static_assert(example::has_default_string() == false, "");

    example ex;
    CHECK(ex.has_default_map() == true);
    CHECK(ex.has_default_unsigned() == true);
    CHECK(ex.has_default_string() == false);
    ex.cb_array(1, {data, data + 4});
  }
}
