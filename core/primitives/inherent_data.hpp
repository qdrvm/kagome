/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_INHERENT_DATA_HPP
#define KAGOME_INHERENT_DATA_HPP

#include <map>
#include <vector>

#include <boost/iterator_adaptors.hpp>
#include <optional>
#include <outcome/outcome.hpp>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/outcome_throw.hpp"
#include "scale/scale.hpp"
#include "scale/scale_error.hpp"

namespace kagome::primitives {
  /**
   * @brief inherent data encode/decode error codes
   */
  enum class InherentDataError {
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
      auto [it, inserted] =
          data.try_emplace(std::move(identifier), common::Buffer());
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

    bool operator==(const InherentData &rhs) const;

    bool operator!=(const InherentData &rhs) const;

    std::map<InherentIdentifier, common::Buffer> data;
  };

  /**
   * @brief output InherentData object instance to stream
   * @tparam Stream stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const InherentData &v) {
    const auto &data = v.data;
    std::vector<std::pair<InherentIdentifier, common::Buffer>> vec;
    vec.reserve(data.size());
    for (auto &pair : data) {
      vec.emplace_back(pair);
    }
    s << vec;
    return s;
  }

  /**
   * @brief decodes InherentData object instance from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, InherentData &v) {
    std::vector<std::pair<InherentIdentifier, common::Buffer>> vec;
    s >> vec;

    for (const auto &item : vec) {
      // throw if identifier already exists
      if (v.data.find(item.first) != v.data.end()) {
        common::raise(InherentDataError::IDENTIFIER_ALREADY_EXISTS);
      }
      v.data.insert(item);
    }

    return s;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_INHERENT_DATA_HPP
