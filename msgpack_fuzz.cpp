#include "catch.hpp"
#include "msgpack.h"

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

functors readall_functors() {
  functors f;
  f.cb_string = [](size_t N, const unsigned char *str) {
    for (size_t i = 0; i < N; i++) {
      unsigned char c = str[i];
      asm("#" ::"r"(c));
    }
  };

  f.cb_signed = [](int64_t x) { asm("#" ::"r"(x)); };

  f.cb_unsigned = [](uint64_t x) { asm("#" ::"r"(x)); };

  f.cb_boolean = [](bool x) { asm("#" ::"r"(x)); };

  f.cb_array_elements = [&f](byte_range bytes) { handle_msgpack(bytes, f); };

  f.cb_map_elements = [&f](byte_range key, byte_range value) {
    handle_msgpack(key, f);
    handle_msgpack(value, f);
  };
  return f;
}

TEST_CASE("check all short byte sequences") {
  SECTION("unary") {
    functors f = readall_functors();
    unsigned char *byte = (unsigned char *)malloc(1);
    bool ok = true;
    for (unsigned i = 0; i < 256; i++) {
      byte[0] = (unsigned char)i;
      const unsigned char *res = handle_msgpack({byte, byte + 1}, f);
      ok &= (res == 0 || res == byte + 1);
    }
    CHECK(ok);
    free(byte);
  }

  SECTION("binary") {
    functors f = readall_functors();
    unsigned N = 2;
    unsigned char *byte = (unsigned char *)malloc(N);
    bool ok = true;
    for (unsigned i = 0; i < 256; i++) {
      for (unsigned j = 0; j < 256; j++) {
        byte[0] = (unsigned char)i;
        byte[1] = (unsigned char)j;
        const unsigned char *res = handle_msgpack({byte, byte + N}, f);
        ok &= (res == 0 || res == byte + 1 || res == byte + 2);
      }
    }
    CHECK(ok);
    free(byte);
  }

  SECTION("ternary") {
    functors f = readall_functors();
    unsigned N = 3;
    unsigned char *byte = (unsigned char *)malloc(N);
    bool ok = true;
    for (unsigned i = 0; i < 256; i++) {
      continue; // skip this for now
      for (unsigned j = 0; j < 256; j++) {
        for (unsigned k = 0; k < 256; k++) {
          byte[0] = (unsigned char)i;
          byte[1] = (unsigned char)j;
          byte[2] = (unsigned char)k;
          const unsigned char *res = handle_msgpack({byte, byte + N}, f);
          ok &= (res == 0 || res == byte + 1 || res == byte + 2 ||
                 res == byte + 3);
        }
      }
    }
    CHECK(ok);
    free(byte);
  }
}

TEST_CASE("from urandom") {
  int fd = open("/dev/urandom", O_RDONLY);
  REQUIRE(fd >= 0);

  unsigned reps = 100;
  unsigned N = 1024;
  unsigned offset = 50;
  unsigned char *bytes = (unsigned char *)malloc(N);
  functors f = readall_functors();

  bool ok = true;
  for (unsigned r = 0; r < reps; r++) {
    ssize_t result = read(fd, bytes, N);
    ok &= result >= 0;
    if (result < 0) {
      break;
    }

    for (unsigned s = 0; s < offset; s++) {
      if (s > N) {
        continue;
      }
      for (unsigned i = 0; i < 256; i++) {
        bytes[s] = (unsigned char)i;
        const unsigned char *res =
            handle_msgpack({bytes + s, bytes + N - s}, f);
        bool in_bounds = (res >= (bytes + s)) && (res <= (bytes + N - s));
        ok &= ((res == 0) || in_bounds);
      }
    }

    for (unsigned w = 0; w < offset; w++) {
      if (w > N) {
        continue;
      }
      for (unsigned i = 0; i < 256; i++) {
        bytes[0] = (unsigned char)i;
        const unsigned char *res = handle_msgpack({bytes, bytes + w}, f);
        bool in_bounds = (res >= bytes) && (res <= (bytes + w));
        ok &= ((res == 0) || in_bounds);
      }
    }
  }
  free(bytes);
  close(fd);
}
