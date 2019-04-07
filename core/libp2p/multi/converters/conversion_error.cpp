/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/conversion_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi::converters, ConversionError, e) {
  using libp2p::multi::converters::ConversionError;

  switch (e) {
    case ConversionError::kInvalidAddress:
      return "Invalid address";
    case ConversionError::kNoSuchProtocol:
      return "Protocol with given code does not exist";
    case ConversionError::kAddressDoesNotBeginWithSlash:
      return "An address should begin with a slash";
    case ConversionError::kNotImplemented:
      return "Conversion for this protocol is not implemented";
    default:
      return "Unknown error";
  }
}