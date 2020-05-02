#include "msgpack.h"
#include "catch.hpp"

extern "C" const unsigned char *nop_handle_msgpack(const unsigned char *start,
                                                   const unsigned char *end) {
  functors f;
  return handle_msgpack({start, end}, f);
}

extern "C" const unsigned char *nop_handle_msgpack_templated(const unsigned char *start,
                                                   const unsigned char *end) { 
  return handle_msgpack({start, end}, functors_nop());
}


extern "C" const unsigned char *nop_handle_msgpack_nonested(const unsigned char *start,
                                                   const unsigned char *end) { 
  return handle_msgpack({start, end}, functors_ignore_nested());
}


extern "C"
void apply_if_top_level_is_unsigned(const unsigned char *start,
                                    const unsigned char *end,
                                    std::function<void(uint64_t)> f)
{
  only_apply_if_top_level_is_unsigned x(f);
  handle_msgpack({start, end}, x);
}

struct example : public functors_defaults<example> {
  void skip_handle_string(size_t N, const unsigned char *str) {
    printf("Called derived handle string\n");
  }

  void handle_array_elements(byte_range bytes) {
    printf("called derived array elements\n");
  }
};


extern "C" const unsigned char *expt(const unsigned char *start,
                                     const unsigned char *end) {

  example ex;
  ex.cb_string(end-start, start);
  ex.cb_unsigned(4);
  return 0;
}
 

TEST_CASE(" ex ")
{

  SECTION("")
    {
      const unsigned char * data  = (const unsigned char*)"foobar\n";
      expt(data,data+10);

      example ex;
      ex.cb_array(1, {data, data + 4});
    }
}
