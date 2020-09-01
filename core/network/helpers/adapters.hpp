/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_HPP
#define KAGOME_ADAPTERS_HPP

#include <functional>
#include <memory>
#include <gsl/span>

namespace kagome::network {

  template<typename T> struct ProtobufMessageAdapter {
    static size_t size(const T &t) {
      assert(!"No implementation");
      return 0ull;
    }
    static std::vector<uint8_t>::iterator write(const T &/*t*/, std::vector<uint8_t> &out, std::vector<uint8_t>::iterator /*loaded*/) {
      assert(!"No implementation");
      return out.end();
    }
  };

  template<typename T> struct UVarMessageAdapter {
    static constexpr size_t kContinuationBitMask{0x80ull};
    static constexpr size_t kSignificantBitsMask{0x7Full};
    static constexpr size_t kSignificantBitsMaskMSB{kSignificantBitsMask << 56};

    static size_t size(const T &t) {
      return sizeof(uint64_t);
    }

    static std::vector<uint8_t>::iterator write(const T &t, std::vector<uint8_t> &out, std::vector<uint8_t>::iterator loaded) {
      const auto remains = std::distance(out.begin(), loaded);
      assert(remains >= UVarMessageAdapter<T>::size(t));

      auto data_sz = out.size() - remains;
      while ((data_sz & kSignificantBitsMaskMSB) == 0 && data_sz != 0)
        data_sz <<= size_t(7ull);

      auto sz_start = --loaded;
      do {
        assert(std::distance(out.begin(), loaded) >= 1);
        *loaded-- = ((data_sz & kSignificantBitsMaskMSB) >> 55ull) | kContinuationBitMask;
      } while(data_sz <<= size_t(7ull));

      *sz_start &= uint8_t(kSignificantBitsMask);
      return ++loaded;
    }

    static libp2p::outcome::result<std::vector<uint8_t>::const_iterator> read(const std::vector<uint8_t> &src, std::vector<uint8_t>::const_iterator from) {
      if (from == src.end())
        return outcome::failure(boost::system::error_code{});

      uint64_t sz = 0;
      constexpr size_t kPayloadSize = ((UVarMessageAdapter<T>::size() << size_t(3)) + size_t(6)) / size_t(7);
      const auto loaded = std::distance(src.begin(), from);

      auto const * const beg = &*from;
      auto const * const end = &src[std::min(loaded + kPayloadSize, src.size())];
      auto const * ptr = beg;
      size_t counter = 0;

      do {
        sz |= (static_cast<uint64_t>(*ptr & kSignificantBitsMask) << (size_t(7) * counter++));
      } while(uint8_t(0) != (*ptr++ & uint8_t(kContinuationBitMask)) && ptr != end);

      if (sz != src.size() - (loaded + counter))
        return outcome::failure(boost::system::error_code{});

      std::advance(from, counter);
      return from;
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_HPP
