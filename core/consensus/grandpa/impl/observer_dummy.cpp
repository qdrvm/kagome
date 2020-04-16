/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
