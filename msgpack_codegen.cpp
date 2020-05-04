#include "catch.hpp"
#include "msgpack.h"

extern "C" const unsigned char *nop_handle_msgpack(const unsigned char *start,
                                                   const unsigned char *end) {
  functors f;
  return handle_msgpack({start, end}, f);
}

extern "C" void nop_handle_msgpack_templated(const unsigned char *start,
                                             const unsigned char *end) {
  handle_msgpack_void({start, end}, functors_nop());
}

extern "C" const unsigned char *
nop_handle_msgpack_nonested(const unsigned char *start,
                            const unsigned char *end) {
  return handle_msgpack({start, end}, functors_ignore_nested());
}

extern "C" void apply_if_top_level_is_unsigned(const unsigned char *start,
                                               const unsigned char *end,
                                               void (*stateless)(uint64_t,void*), void * st) {

  only_apply_if_top_level_is_unsigned x(stateless, st);
  handle_msgpack_void({start, end}, x);
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
