#include "msgpack.h"

extern "C" const unsigned char *nop_handle_msgpack(const unsigned char *start,
                                                   const unsigned char *end) {
  functors f;
  return handle_msgpack({start, end}, f);
}
