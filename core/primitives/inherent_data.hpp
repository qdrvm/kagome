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
#include "common/buffer.hpp"
#include "common/blob.hpp"

namespace kagome::primitives {

  using InherentIdentifier = common::Blob<8u>;

  /**
   * Inherent data to include in a block
   */
  class InherentData {
   public:
    enum class Error { IDENTIFIER_ALREADY_EXISTS = 1 };

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
}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, InherentData::Error);

#endif  // KAGOME_INHERENT_DATA_HPP
