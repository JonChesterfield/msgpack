#ifndef MSGPACK_HPP
#define MSGPACK_HPP

#include <functional>

namespace fallback {

unsigned char *skip_next_message(unsigned char *start, unsigned char *end);

void nop_string(size_t, unsigned char *);
void nop_signed(int64_t);
void nop_unsigned(uint64_t);
void nop_boolean(bool);
void nop_array_elements(unsigned char *, unsigned char *);
void nop_map_elements(unsigned char *, unsigned char *, unsigned char *,
                      unsigned char *);

unsigned char *nop_map(uint64_t N, unsigned char *start, unsigned char *end);
unsigned char *nop_array(uint64_t N, unsigned char *start, unsigned char *end);

unsigned char *
array(uint64_t N, unsigned char *start, unsigned char *end,
      std::function<void(unsigned char *, unsigned char *)> callback);
unsigned char *map(uint64_t N, unsigned char *start, unsigned char *end,
                   std::function<void(unsigned char *, unsigned char *,
                                      unsigned char *, unsigned char *)>
                       callback);
} // namespace fallback

struct functors {

  std::function<void(size_t, unsigned char *)> cb_string = fallback::nop_string;

  std::function<void(int64_t)> cb_signed = fallback::nop_signed;

  std::function<void(uint64_t)> cb_unsigned = fallback::nop_unsigned;

  std::function<void(bool)> cb_boolean = fallback::nop_boolean;

  std::function<void(unsigned char *, unsigned char *, unsigned char *,
                     unsigned char *)>
      cb_map_elements = fallback::nop_map_elements;

  std::function<void(unsigned char *, unsigned char *)> cb_array_elements =
      fallback::nop_array_elements;

  std::function<unsigned char *(uint64_t N, unsigned char *start,
                                unsigned char *end)>
      cb_array = [=](uint64_t N, unsigned char *start, unsigned char *end) {
        return fallback::array(N, start, end, this->cb_array_elements);
      };

  std::function<unsigned char *(uint64_t N, unsigned char *start,
                                unsigned char *end)>

      cb_map = [=](uint64_t N, unsigned char *start, unsigned char *end) {
        return fallback::map(N, start, end, this->cb_map_elements);
      };
};

unsigned char *handle_msgpack(unsigned char *start, unsigned char *end,
                              functors f);

void foreach_map(unsigned char *start, unsigned char *end,
                 std::function<void(unsigned char *, unsigned char *,
                                    unsigned char *, unsigned char *)>
                 callback);

void foreach_array(
                   unsigned char *start, unsigned char *end,
    std::function<void(unsigned char *, unsigned char *)> callback);


#endif
