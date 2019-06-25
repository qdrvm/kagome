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

  inline bool operator==(const NetworkAddress &l, const NetworkAddress &r) {
    return l.ip_version == r.ip_version && l.port == r.port;
  }
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_NETWORK_ADDRESS_HPP
