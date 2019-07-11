/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_INHERENT_DATA_HPP
#define KAGOME_INHERENT_DATA_HPP

#include <map>
#include <optional>
#include <vector>

#include <boost/iterator_adaptors.hpp>
#include <outcome/outcome.hpp>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "scale/outcome_throw.hpp"
#include "scale/scale_error.hpp"

namespace kagome::primitives {

  using InherentIdentifier = common::Blob<8u>;

  /**
   * @brief inherent data encode/decode error codes
   */
  enum class InherentDataError { IDENTIFIER_ALREADY_EXISTS = 1 };

  /**
   * Inherent data to include in a block
   */
  class InherentData {
   public:
    /** Put data for an inherent into the internal storage.
     *
     * @arg identifier need to be unique, otherwise decoding of these
     * values will not work!
     * @arg inherent encoded data to be stored
     * @returns success if the data could be inserted an no data for an inherent
     * with the same
     */
    outcome::result<void> putData(InherentIdentifier identifier,
                                  common::Buffer inherent);

    /** Replace the data for an inherent.
     * If it does not exist, the data is just inserted.
     * @arg inherent encoded data to be stored
     */
    void replaceData(InherentIdentifier identifier, common::Buffer inherent);

    /**
     * @returns the data for the requested inherent.
     */
    outcome::result<std::optional<common::Buffer>> getData(
        const InherentIdentifier &identifier) const;

    const std::map<InherentIdentifier, common::Buffer> &getDataCollection()
        const;

   private:
    std::map<InherentIdentifier, common::Buffer> data_;
  };

  /**
   * @brief output InherentData object instance to stream
   * @tparam Stream stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream>
  Stream &operator<<(Stream &s, const InherentData &v) {
    const auto &data = v.getDataCollection();
    // vectors
    std::vector<std::reference_wrapper<const InherentIdentifier>> ids;
    ids.reserve(data.size());
    std::vector<std::reference_wrapper<const common::Buffer>> vals;
    vals.reserve(data.size());

    for (auto &pair : data) {
      ids.push_back(std::ref(pair.first));
      vals.push_back(std::ref(pair.second));
    }

    s << ids << vals;
    return s;
  }

  /**
   * @brief decodes InherentData object instance from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream>
  Stream &operator>>(Stream &s, InherentData &v) {
    std::vector<InherentIdentifier> ids;
    std::vector<common::Buffer> vals;
    s >> ids >> vals;
    if (ids.size() != vals.size()) {
      scale::common::raise(scale::DecodeError::INVALID_DATA);
    }

    for (size_t i = 0u; i < ids.size(); ++i) {
      auto &&res = v.putData(ids[i], vals[i]);
      if (!res) {
        scale::common::raise(InherentDataError::IDENTIFIER_ALREADY_EXISTS);
      }
    }

    return s;
  }
}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, InherentDataError);

#endif  // KAGOME_INHERENT_DATA_HPP
