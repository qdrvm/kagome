/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <optional>
#include <vector>

#include <scale/scale.hpp>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/outcome_throw.hpp"

namespace kagome::primitives {
  /**
   * @brief inherent data encode/decode error codes
   */
  enum class InherentDataError : uint8_t {
    IDENTIFIER_ALREADY_EXISTS = 1,
    IDENTIFIER_DOES_NOT_EXIST
  };
}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, InherentDataError);

namespace kagome::primitives {
  using InherentIdentifier = common::Blob<8u>;

  /**
   * Inherent data to include in a block
   */
  struct InherentData {
    /** Put data for an inherent into the internal storage.
     *
     * @arg identifier need to be unique, otherwise decoding of these
     * values will not work!
     * @arg inherent encoded data to be stored
     * @returns success if the data could be inserted an no data for an inherent
     * with the same
     */
    template <typename T>
    outcome::result<void> putData(InherentIdentifier identifier,
                                  const T &inherent) {
      auto [it, inserted] = data.try_emplace(identifier, common::Buffer());
      if (inserted) {
        it->second = common::Buffer(scale::encode(inherent).value());
        return outcome::success();
      }
      return InherentDataError::IDENTIFIER_ALREADY_EXISTS;
    }

    /** Replace the data for an inherent.
     * If it does not exist, the data is just inserted.
     * @arg inherent encoded data to be stored
     */
    template <typename T>
    void replaceData(InherentIdentifier identifier, const T &inherent) {
      data[identifier] = common::Buffer(scale::encode(inherent).value());
    }

    /**
     * @returns the data for the requested inherent.
     */
    template <typename T>
    outcome::result<T> getData(const InherentIdentifier &identifier) const {
      auto inherent = data.find(identifier);
      if (inherent != data.end()) {
        auto buf = inherent->second;
        return scale::decode<T>(buf);
      }
      return InherentDataError::IDENTIFIER_DOES_NOT_EXIST;
    }

    std::map<InherentIdentifier, common::Buffer> data;

    bool operator==(const InherentData &) const = default;
  };

  /**
   * @brief decodes InherentData object instance from stream
   * @param decoder stream reference
   * @param v value to decode
   * @return reference to stream
   */
  inline void decode(InherentData &v, scale::Decoder &decoder) {
    std::vector<std::pair<InherentIdentifier, common::Buffer>> vec;
    decode(vec, decoder);
    for (const auto &pair : vec) {
      if (not v.data.emplace(pair).second) {
        common::raise(InherentDataError::IDENTIFIER_ALREADY_EXISTS);
      }
    }
  }
}  // namespace kagome::primitives
