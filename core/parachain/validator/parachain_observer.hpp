/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PARACHAINOBSERVER
#define KAGOME_PARACHAIN_PARACHAINOBSERVER

#include "network/collation_observer.hpp"
#include "network/req_collation_observer.hpp"
#include "network/req_pov_observer.hpp"
#include "network/validation_observer.hpp"

namespace kagome::parachain {

  class ParachainObserver : public network::CollationObserver,
                            public network::ValidationObserver,
                            public network::ReqCollationObserver,
                            public network::ReqPovObserver {};

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PARACHAINOBSERVER
