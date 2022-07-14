/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_UVAR
#define KAGOME_ADAPTERS_UVAR

#include <boost/system/error_code.hpp>
#include <functional>
#include <gsl/span>
#include <memory>
#include <vector>

#include "network/adapters/adapter_errors.hpp"
#include "outcome/outcome.hpp"

namespace kagome::network {

  template <typename T>
  struct UVarMessageAdapter {
    static constexpr size_t kContinuationBitMask{0x80ull};
    static constexpr size_t kSignificantBitsMask{0x7Full};
    static constexpr size_t kMSBBit = (1ull << 63);
    static constexpr size_t kPayloadSize =
        ((sizeof(uint64_t) << size_t(3)) + size_t(6)) / size_t(7);
    static constexpr size_t kSignificantBitsMaskMSB{kSignificantBitsMask << 57};

    static size_t size(const T & /*t*/) {
      return kPayloadSize;
    }

    static std::vector<uint8_t>::iterator write(
        const T &t,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator loaded) {
      const auto remains =
          static_cast<size_t>(std::distance(out.begin(), loaded));
      assert(remains >= UVarMessageAdapter<T>::size(t));
      auto data_sz = out.size() - remains;

      /// We need to align bits to 7-bit bound
      if (0 == (kMSBBit & data_sz)) {
        data_sz <<= size_t(1ull);
        while ((data_sz & kSignificantBitsMaskMSB) == 0 && data_sz != 0)
          data_sz <<= size_t(7ull);
      }

      auto sz_start = --loaded;
      do {
        assert(std::distance(out.begin(), loaded) >= 1);
        *loaded-- = ((data_sz & kSignificantBitsMaskMSB) >> 57ull)
                    | kContinuationBitMask;
      } while ((data_sz <<= size_t(7ull)) != 0);

      *sz_start &= uint8_t(kSignificantBitsMask);
      return ++loaded;
    }

    static outcome::result<std::vector<uint8_t>::const_iterator> read(
        T & /*out*/,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      if (from == src.end()) return AdaptersError::EMPTY_DATA;

      uint64_t sz = 0;
      const auto loaded = std::distance(src.begin(), from);

      auto const *const beg = from.base();
      auto const *const end = &src[std::min(loaded + kPayloadSize, src.size())];
      auto const *ptr = beg;
      size_t counter = 0;

      do {
        sz |= (static_cast<uint64_t>(*ptr & kSignificantBitsMask)
               << (size_t(7) * counter++));
      } while (uint8_t(0) != (*ptr++ & uint8_t(kContinuationBitMask))  // NOLINT
               && ptr != end);

      if (sz != src.size() - (loaded + counter))
        return AdaptersError::DATA_SIZE_CORRUPTED;

      std::advance(from, counter);
      return from;
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_UVAR
