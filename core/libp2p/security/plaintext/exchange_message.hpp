/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXCHANGE_MESSAGE_HPP
#define KAGOME_EXCHANGE_MESSAGE_HPP

#include "libp2p/crypto/key.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::security::plaintext {

  struct ExchangeMessage {
    crypto::PublicKey pubkey;
    peer::PeerId peer_id;
  };

}

#endif  // KAGOME_EXCHANGE_MESSAGE_HPP
