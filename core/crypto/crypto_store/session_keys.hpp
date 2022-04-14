/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SESSION_KEYS_HPP
#define KAGOME_CRYPTO_SESSION_KEYS_HPP

#include "common/blob.hpp"
#include "crypto/crypto_store/key_type.hpp"
#include "network/types/roles.hpp"

namespace kagome::crypto {

  class CryptoStore;
  struct Ed25519Keypair;
  struct Sr25519Keypair;

  // hardcoded keys order for polkadot
  // otherwise it could be read from chainspec palletSession/keys
  // nevertheless they are hardcoded in polkadot
  // https://github.com/paritytech/polkadot/blob/634520cd3cf4b2b850db807daaaa32e480099981/node/service/src/chain_spec.rs#L230
  constexpr KnownKeyTypeId polkadot_key_order[6]{KEY_TYPE_GRAN,
                                                 KEY_TYPE_BABE,
                                                 KEY_TYPE_IMON,
                                                 KEY_TYPE_PARA,
                                                 KEY_TYPE_ASGN,
                                                 KEY_TYPE_AUDI};

  class SessionKeys {
    std::shared_ptr<Sr25519Keypair> babe_key_pair_;
    std::shared_ptr<Ed25519Keypair> gran_key_pair_;
    network::Roles roles_;
    std::shared_ptr<CryptoStore> store_;

   public:
    SessionKeys(std::shared_ptr<CryptoStore> store,
                const network::Roles &roles);

    /**
     * @return current BABE session key pair
     */
    const std::shared_ptr<Sr25519Keypair> &getBabeKeyPair();

    /**
     * @return current GRANDPA session key pair
     */
    const std::shared_ptr<Ed25519Keypair> &getGranKeyPair();
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_SESSION_KEYS_HPP
