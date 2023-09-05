#include "crypto/hasher/blake2b_stream_hasher.hpp"

namespace kagome::crypto {
  Blake2b_StreamHasher::Blake2b_StreamHasher(size_t outlen) : outlen_{outlen} {}

  bool Blake2b_StreamHasher::update(gsl::span<const uint8_t> buffer) {
    if (!initialized_) {
      if (blake2b_init(&ctx_, outlen_, nullptr, 0ull)) {
        return false;
      }
      initialized_ = true;
    }
    blake2b_update(&ctx_, buffer.data(), buffer.size());
    return true;
  }

  bool Blake2b_StreamHasher::get_final(gsl::span<uint8_t> out) {
    if (out.size() < (decltype(out)::index_type)outlen_) {
      return false;
    }
    blake2b_final(&ctx_, out.data());
    return true;
  }
}  // namespace kagome::crypto
