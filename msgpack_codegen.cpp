#include "catch.hpp"
#include "msgpack.h"

using namespace msgpack;

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
