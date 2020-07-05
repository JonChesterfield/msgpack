#include "catch.hpp"
#include "msgpack.h"

#include <endian.h>

namespace {
bool type_can_encode(msgpack::type ty, uint64_t value) {
  using namespace msgpack;
  switch (ty) {
  default:
    return false;
  case posfixint:
    return value <= 127u;
  case uint8:
    return value <= UINT8_MAX;
  case uint16:
    return value <= UINT16_MAX;
  case uint32:
    return value <= UINT32_MAX;
  case uint64:
    return true;
  }
}

bool type_can_encode(msgpack::type ty, int64_t value) {
  using namespace msgpack;
  switch (ty) {
  default:
    return false;
  case negfixint: {
    // Can't represent > 8 bit values
    int8_t trunc = (int8_t)value;
    if (value != (int64_t)trunc) {
      return false;
    }

    // negative fixint represents 111xxxxx
    uint8_t utrunc;
    __builtin_memcpy(&utrunc, &trunc, 1);
    return (utrunc >> 5) == 7;
  }
  case int8:
    return value >= INT8_MIN && value <= INT8_MAX;
  case int16:
    return value >= INT16_MIN && value <= INT16_MAX;
  case int32:
    return value >= INT32_MIN && value <= INT32_MAX;
  case int64:
    return true;
  }
}

template <typename T>
bool can_encode(msgpack::type ty, T value, unsigned char *start,
                unsigned char *end) {
  if (!type_can_encode(ty, value)) {
    return false;
  }
  uint64_t available = end - start;
  if (available < bytes_used_fixed(ty)) {
    return false;
  }
  return true;
}

unsigned char *encode(msgpack::type ty, uint64_t value, unsigned char *start,
                      unsigned char *end) {
  using namespace msgpack;
  if (!can_encode(ty, value, start, end)) {
    return nullptr;
  }

  switch (ty) {
  default: {
    return nullptr;
  }
  case posfixint: {
    *start++ = (uint8_t)value;
    return start;
  }
  case uint8: {
    *start++ = UINT8_C(0xcc);
    *start = value;
    return start + 1;
  }
  case uint16: {
    *start++ = UINT8_C(0xcd);
    uint16_t s = htobe16((uint16_t)value);
    memcpy(start, &s, 2);
    return start + 2;
  }
  case uint32: {
    *start++ = UINT8_C(0xce);
    uint32_t s = htobe32((uint32_t)value);
    memcpy(start, &s, 4);
    return start + 4;
  }
  case uint64: {
    *start++ = UINT8_C(0xcf);
    uint64_t s = htobe64((uint64_t)value);
    memcpy(start, &s, 8);
    return start + 8;
  }
  }
}

unsigned char *encode(msgpack::type ty, int64_t value, unsigned char *start,
                      unsigned char *end) {
  using namespace msgpack;
  if (!can_encode(ty, value, start, end)) {
    return nullptr;
  }
  switch (ty) {
  default: {
    return nullptr;
  }
  case negfixint: {
    int8_t t = (int8_t)value;
    memcpy(start, &t, 1);
    return start + 1;
  }
  case int8: {
    *start++ = UINT8_C(0xd0);
    int8_t t = (int8_t)value;
    memcpy(start, &t, 1);
    return start + 1;
  }
  case int16: {
    *start++ = UINT8_C(0xd1);
    int16_t t = (int16_t)value;
    uint16_t r;
    memcpy(&r, &t, 2);
    r = htobe16(r);
    memcpy(start, &r, 2);
    return start + 2;
  }
  case int32: {
    *start++ = UINT8_C(0xd2);
    int32_t t = (int32_t)value;
    uint32_t r;
    memcpy(&r, &t, 4);
    r = htobe32(r);
    memcpy(start, &r, 4);
    return start + 4;
  }
  case int64: {
    *start++ = UINT8_C(0xd3);
    int64_t t = (int64_t)value;
    uint64_t r;
    memcpy(&r, &t, 8);
    r = htobe64(r);
    memcpy(start, &r, 8);
    return start + 8;
  }
  }
}
}
using namespace msgpack;

TEST_CASE("unsigned") {
  SECTION("given sufficient buffer") {
    unsigned char buffer[16];

    auto write = [&](type ty, uint64_t value) {
      unsigned char *r = encode(ty, value, buffer, buffer + sizeof(buffer));
      CHECK(r == buffer + bytes_used_fixed(ty));
    };

    auto read = [&](uint64_t expect) -> bool {
      for (uint64_t parsed : {UINT64_C(0), UINT64_MAX}) {
        uint64_t count = 0;
        foronly_unsigned({buffer, buffer + sizeof(buffer)}, [&](uint64_t x) {
          count++;
          parsed = x;
        });

        if (count != 1) {
          return false;
        }
        if (parsed != expect) {
          return false;
        }
      }

      return true;
    };

    for (uint64_t v :
         (uint64_t[]){0, 1, UINT8_MAX, UINT8_MAX + 1, UINT16_MAX,
                      UINT16_MAX + 1, UINT32_MAX, UINT32_MAX + 1, UINT64_MAX}) {
      for (type ty : {posfixint, uint8, uint16, uint32, uint64}) {
        if (type_can_encode(ty, v)) {
          write(ty, v);
          CHECK(read(v));
        }
      }
    }
  }
}

TEST_CASE("signed") {
  SECTION("given sufficient buffer") {
    unsigned char buffer[16];

    auto write = [&](type ty, int64_t value) {
      unsigned char *r = encode(ty, value, buffer, buffer + sizeof(buffer));
      CHECK(r == buffer + bytes_used_fixed(ty));
    };

    auto read = [&](int64_t expect) -> bool {
      for (int64_t parsed : {INT64_MIN, INT64_C(0), INT64_MAX}) {
        uint64_t count = 0;
        foronly_signed({buffer, buffer + sizeof(buffer)}, [&](int64_t x) {
          count++;
          parsed = x;
        });

        if (count != 1) {
          return false;
        }
        if (parsed != expect) {
          return false;
        }
      }

      return true;
    };

    for (int64_t v :
         (int64_t[]){-1, 0, 1, INT8_MIN - 1, INT8_MIN, INT8_MAX, INT8_MAX + 1,
                     INT16_MIN - 1, INT16_MIN, INT16_MAX, INT16_MAX + 1,
                     INT32_MIN - 1, INT32_MIN, INT32_MAX, INT32_MAX + 1,
                     INT64_MIN, INT64_MIN + 1, INT64_MAX - 1, INT64_MAX}) {
      for (type ty : {negfixint, int8, int16, int32, int64}) {
        if (type_can_encode(ty, v)) {
          write(ty, v);
          CHECK(read(v));
        }
      }
    }
  }
}
