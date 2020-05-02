#include "catch.hpp"
#include "msgpack.h"

extern "C" const unsigned char *nop_handle_msgpack(const unsigned char *start,
                                                   const unsigned char *end) {
  functors f;
  return handle_msgpack({start, end}, f);
}

extern "C" const unsigned char *
nop_handle_msgpack_templated(const unsigned char *start,
                             const unsigned char *end) {
  return handle_msgpack({start, end}, functors_nop());
}

extern "C" const unsigned char *
nop_handle_msgpack_nonested(const unsigned char *start,
                            const unsigned char *end) {
  return handle_msgpack({start, end}, functors_ignore_nested());
}

extern "C" void
apply_if_top_level_is_unsigned(const unsigned char *start,
                               const unsigned char *end,
                               std::function<void(uint64_t)> f) {
  only_apply_if_top_level_is_unsigned x(f);
  handle_msgpack({start, end}, x);
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

    static_assert(example::handle_string_is_default() == false, "");

    example ex;
    CHECK(ex.handle_map_is_default() == true);
    CHECK(ex.handle_unsigned_is_default() == true);
    CHECK(ex.handle_string_is_default() == false);
    ex.cb_array(1, {data, data + 4});
  }
}
