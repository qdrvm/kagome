/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/impl/address_publisher_errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::authority_discovery,
                            AddressPublisherError,
                            e) {
  using E = kagome::authority_discovery::AddressPublisherError;
  switch (e) {
    case E::NO_GRAND_KEY:
      return "GRANDPA key is missing";
    case E::NO_AUDI_KEY:
      return "AUDI key is missing";
    case E::NO_AUTHORITY:
      return "Authority is missing";
    case E::WRONG_KEY_TYPE:
      return "Libp2p key expected type is Ed25519";
  }
  return "Unknown error";
}
