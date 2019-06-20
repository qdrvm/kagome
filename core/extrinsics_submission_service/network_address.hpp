/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_NETWORK_ADDRESS_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_NETWORK_ADDRESS_HPP

#include <cstdint>

namespace kagome::service {
  enum class IpVersion { IPV4, IPV6 };

  struct NetworkAddress {
    IpVersion ip_version{IpVersion::IPV4};
    uint16_t port{0};
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_NETWORK_ADDRESS_HPP
