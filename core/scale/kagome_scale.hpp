/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAGOME_SCALE_HPP
#define KAGOME_KAGOME_SCALE_HPP

#include "common/blob.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"
#include "scale/encode_append.hpp"

namespace kagome::scale {
  using CompactInteger = ::scale::CompactInteger;

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeader &bh);

  template <typename F, typename ElementType, size_t MaxSize, typename... Args>
  constexpr void encode(
      const F &func, const common::SLVector<ElementType, MaxSize, Args...> &c);

  template <typename F, typename T, typename Tag, typename Base>
  constexpr void encode(const F &func, const Tagged<T, Tag, Base> &c);

  template <typename F, size_t MaxSize>
  constexpr void encode(const F &func, const common::SLBuffer<MaxSize> &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::Other &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::Consensus &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::Seal &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c);

  template <typename F>
  constexpr void encode(const F &func,
                        const primitives::RuntimeEnvironmentUpdated &c);

  template <typename F>
  constexpr void encode(const F &func, const ::scale::EncodeOpaqueValue &c);

  template <typename F>
  constexpr void encode(const F &func,
                        const consensus::babe::BabeBlockHeader &bh);

}  // namespace kagome::scale

#include "scale/encoder/primitives.hpp"

namespace kagome::scale {

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeader &bh) {
    encode(func, bh.parent_hash);
    encode(func, ::scale::CompactInteger(bh.number));
    encode(func, bh.state_root);
    encode(func, bh.extrinsics_root);
    encode(func, bh.digest);
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const consensus::babe::BabeBlockHeader &bh) {
    encode(func, bh.slot_assignment_type);
    encode(func, bh.authority_index);
    encode(func, bh.slot_number);

    if (bh.needVRFCheck()) {
      encode(func, bh.vrf_output);
    }
  }

  template <typename F, typename ElementType, size_t MaxSize, typename... Args>
  constexpr void encode(
      const F &func, const common::SLVector<ElementType, MaxSize, Args...> &c) {
    encode(func, ::scale::CompactInteger{c.size()});
    encode(func, c.begin(), c.end());
  }

  template <typename F, typename T, typename Tag, typename Base>
  constexpr void encode(const F &func, const Tagged<T, Tag, Base> &c) {
    if constexpr (std::is_scalar_v<T>) {
      encode(func, c.Wrapper<T>::value);
    } else {
      encode(func, static_cast<const T &>(c));
    }
  }

  template <typename F, size_t MaxSize>
  constexpr void encode(const F &func, const common::SLBuffer<MaxSize> &c) {
    encode(func, static_cast<const common::SLVector<uint8_t, MaxSize> &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Other &c) {
    encode(func, static_cast<const common::Buffer &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Consensus &c) {
    encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Seal &c) {
    encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c) {
    encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const primitives::RuntimeEnvironmentUpdated &c) {}

  template <typename F>
  constexpr void encode(const F &func, const ::scale::EncodeOpaqueValue &c) {
    putByte(func, c.v.data(), c.v.size());
  }

}  // namespace kagome::scale

#include "scale/scale.hpp"

template <typename T>
inline std::vector<uint8_t> compareWithRef3(T &&t) {
  std::vector<uint8_t> data_0;

  kagome::scale::encode(
      [&](const uint8_t *const val, size_t count) {
        for (size_t i = 0; i < count; ++i) {
          data_0.emplace_back(val[i]);
        }
      },
      t);

  std::vector<uint8_t> data_1 = ::scale::encode(t).value();
  assert(data_0.size() == data_1.size());
  for (size_t ix = 0; ix < data_0.size(); ++ix) {
    assert(data_0[ix] == data_1[ix]);
  }

  return data_0;
}

template <typename... T>
inline outcome::result<std::vector<uint8_t>> compareWithRef4(T &&...t) {
  std::vector<uint8_t> data_0;
  kagome::scale::encode(
      [&](const uint8_t *const val, size_t count) {
        for (size_t i = 0; i < count; ++i) {
          data_0.emplace_back(val[i]);
        }
      },
      t...);

  std::vector<uint8_t> data_1 = ::scale::encode(t...).value();
  assert(data_0.size() == data_1.size());
  for (size_t ix = 0; ix < data_0.size(); ++ix) {
    assert(data_0[ix] == data_1[ix]);
  }

  return outcome::success(data_0);
}

template <typename T>
inline void compareWithRef(const T &t, const std::vector<uint8_t> &data_1) {
  std::vector<uint8_t> data_0;
  kagome::scale::encode(
      [&](const uint8_t *const val, size_t count) {
        for (size_t i = 0; i < count; ++i) {
          data_0.emplace_back(val[i]);
        }
      },
      t);

  assert(data_0.size() == data_1.size());
  ASSERT_EQ(data_0.size(), data_1.size());
  for (size_t ix = 0; ix < data_0.size(); ++ix) {
    ASSERT_EQ(data_0[ix], data_1[ix]);
  }
}

template <typename T>
inline std::vector<uint8_t> compareWithRef2(const T &t,
                                            std::vector<uint8_t> data_1) {
  std::vector<uint8_t> data_0;
  kagome::scale::encode(
      [&](const uint8_t *const val, size_t count) {
        for (size_t i = 0; i < count; ++i) {
          data_0.emplace_back(val[i]);
        }
      },
      t);

  assert(data_0.size() == data_1.size());
  for (size_t ix = 0; ix < data_0.size(); ++ix) {
    assert(data_0[ix] == data_1[ix]);
  }

  return data_1;
}

#endif  // KAGOME_KAGOME_SCALE_HPP
