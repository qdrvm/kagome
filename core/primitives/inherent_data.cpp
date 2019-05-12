/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/inherent_data.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, InherentData::Error, e) {
  using E = kagome::primitives::InherentData::Error;
  switch (e) {
    case E::IDENTIFIER_ALREADY_EXISTS:
      return "This identifier already exists";
  }
  return "Unknow error";
}

namespace kagome::primitives {

  outcome::result<void> InherentData::putData(InherentIdentifier identifier,
                                              common::Buffer inherent) {
    if (data_.find(identifier) == data_.end()) {
      data_[identifier] = std::move(inherent);
      return outcome::success();

    } else {
      return Error::IDENTIFIER_ALREADY_EXISTS;
    }
  }

  /** Replace the data for an inherent.
   * If it does not exist, the data is just inserted.
   * @arg inherent encoded data to be stored
   */
  void InherentData::replaceData(InherentIdentifier identifier,
                                 common::Buffer inherent) {
    data_[identifier] = std::move(inherent);
  }

  /**
   * @returns the data for the requested inherent.
   */
  outcome::result<std::optional<common::Buffer>> InherentData::getData(
      const InherentIdentifier &identifier) const {
    auto inherent = data_.find(identifier);
    if (inherent != data_.end()) {
      return inherent->second;
    } else {
      return std::nullopt;
    }
  }

  const std::map<InherentIdentifier, common::Buffer>
      &InherentData::getDataCollection() const {
    return data_;
  }

}  // namespace kagome::primitives
