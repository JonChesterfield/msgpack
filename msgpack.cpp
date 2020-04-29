#include "catch.hpp"

extern "C" {
#include "helloworld_msgpack.h"
}

namespace msgpack {
typedef enum {
  posfixint,
  fixmap,
  fixarray,
  fixstr,
  nil,
  f,
  t,
  bin8,
  bin16,
  bin32,
  ext8,
  ext16,
  ext32,
  float32,
  float64,
  uint8,
  uint16,
  uint32,
  uint64,
  int8,
  int16,
  int32,
  int64,
  fixext1,
  fixext2,
  fixext4,
  fixext8,
  fixext16,
  str8,
  str16,
  str32,
  array16,
  array32,
  map16,
  map32,
  negfixint,
  failure,
} type;

}

msgpack::type parse_type(unsigned char x) {
  using namespace msgpack;
  switch(x)
    {
    case  0x00 ... 0x7f:     
      return posfixint;
    case 0x80 ... 0x8f:
      return fixmap;
    case 0x90 ... 0x9f:
      return fixarray;
    case 0xa0 ... 0xbf:
      return fixstr;
    case 0xc0: return nil;
    case 0xc1: return failure;
    case 0xc2: return f;
    case 0xc3: return t;
    case 0xc4: return bin8;
    case 0xc5: return bin16;
    case 0xc6: return bin32;
    case 0xc7: return ext8;
    case 0xc8: return ext16;
    case 0xc9: return ext32;
    case 0xca: return float32;
    case 0xcb: return float64;
    case 0xcc: return uint8;
    case 0xcd: return uint16;
    case 0xce: return uint32;
    case 0xcf: return uint64;
    case 0xd0: return int8;
    case 0xd1: return int16;
    case 0xd2: return int32;
    case 0xd3: return int64;
    case 0xd4: return fixext1;
    case 0xd5: return fixext2;
    case 0xd6: return fixext4;
    case 0xd7: return fixext8;
    case 0xd8: return fixext16;
    case 0xd9: return str8;
    case 0xda: return str16;
    case 0xdb: return str32;
    case 0xdc: return array16;
    case 0xdd: return array32;
    case 0xde: return map16;
    case 0xdf: return map32;
    case 0xe0 ... 0xff: return negfixint;
      
      

        
        
      
      

    default:
      return failure;
    }



}

TEST_CASE("str") { CHECK(helloworld_msgpack_len != 0); }

TEST_CASE("hello world") {
  SECTION("sanity checks") {
    unsigned char start = helloworld_msgpack[0];
    CHECK(parse_type(start) == msgpack::map32);
  }
}
