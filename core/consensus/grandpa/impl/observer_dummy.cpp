/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/observer_dummy.hpp"

namespace kagome::consensus::grandpa {

  ObserverDummy::ObserverDummy()
      : logger_{common::createLogger("ObserverDummy")} {}

  void ObserverDummy::onPrecommit(Precommit) {
    logger_->error("onPrecommit is not implemented");
  }

  void ObserverDummy::onPrevote(Prevote) {
    logger_->error("onPrevote is not implemented");
  }

  void ObserverDummy::onPrimaryPropose(PrimaryPropose) {
    logger_->error("onPrimaryPropose is not implemented");
  }

}  // namespace kagome::consensus::grandpa
