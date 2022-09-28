/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_DISCOVERY_HPP
#define KAGOME_AUTHORITY_DISCOVERY_HPP

namespace kagome::authority_discovery {

  class AddressPublisher {
   public:
    virtual ~AddressPublisher() = default;

    virtual void run() = 0;
  };

}  // namespace kagome::authority_discovery

#endif  // KAGOME_AUTHORITY_DISCOVERY_HPP
