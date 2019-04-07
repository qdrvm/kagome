/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_MULTI_CONVERTERS_CONVERSION_ERROR_HPP_
#define KAGOME_CORE_LIBP2P_MULTI_CONVERTERS_CONVERSION_ERROR_HPP_

#include <outcome/outcome.hpp>

namespace libp2p::multi::converters {

  /**
   * An error that might occur during conversion of
   * a multiaddr between byte format and string format
   */
  enum class ConversionError {
    kAddressDoesNotBeginWithSlash = 1,
    kNoSuchProtocol,
    kInvalidAddress,
    kNotImplemented
  };
}

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi::converters, ConversionError);

#endif  // KAGOME_CORE_LIBP2P_MULTI_CONVERTERS_CONVERSION_ERROR_HPP_
