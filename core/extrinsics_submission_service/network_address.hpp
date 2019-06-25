/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_NETWORK_ADDRESS_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_NETWORK_ADDRESS_HPP

#include <cstdint>
#include <boost/asio/ip/address.hpp>

namespace kagome::service {
  using NetworkAddress = boost::asio::ip::address;
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_NETWORK_ADDRESS_HPP
