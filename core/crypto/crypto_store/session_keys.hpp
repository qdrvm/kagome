/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SESSION_KEYS_HPP
#define KAGOME_CRYPTO_SESSION_KEYS_HPP

#include "common/blob.hpp"
#include "crypto/crypto_store/key_type.hpp"
#include "network/types/roles.hpp"
#include "primitives/authority.hpp"
#include "primitives/authority_discovery_id.hpp"

namespace kagome::application {
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::crypto {

  class CryptoStore;
  struct Ed25519Keypair;
  struct Sr25519Keypair;
  struct Sr25519PublicKey;

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
   public:
    template <typename T>
    using KeypairWithIndexOpt = std::optional<
        std::pair<std::shared_ptr<T>, primitives::AuthorityIndex>>;

    virtual ~SessionKeys() = default;

    /**
     * @return current BABE session key pair
     */
    virtual KeypairWithIndexOpt<Sr25519Keypair> getBabeKeyPair(
        const primitives::AuthorityList &authorities) = 0;

    /**
     * @return current GRANDPA session key pair
     */
    virtual std::shared_ptr<Ed25519Keypair> getGranKeyPair(
        const primitives::AuthoritySet &authorities) = 0;

    /**
     * @return current parachain validator session key pair
     */
    virtual KeypairWithIndexOpt<Sr25519Keypair> getParaKeyPair(
        const std::vector<Sr25519PublicKey> &authorities) = 0;

    /**
     * @return current AUDI session key pair
     */
    virtual std::shared_ptr<Sr25519Keypair> getAudiKeyPair(
        const std::vector<primitives::AuthorityDiscoveryId> &authorities) = 0;
  };

  class SessionKeysImpl : public SessionKeys {
    KeypairWithIndexOpt<Sr25519Keypair> babe_key_pair_;
    KeypairWithIndexOpt<Ed25519Keypair> gran_key_pair_;
    KeypairWithIndexOpt<Sr25519Keypair> para_key_pair_;
    KeypairWithIndexOpt<Sr25519Keypair> audi_key_pair_;
    network::Roles roles_;
    std::shared_ptr<CryptoStore> store_;

    template <typename T>
    using FnListPublic = outcome::result<std::vector<decltype(T::public_key)>> (
        CryptoStore::*)(KeyTypeId) const;
    template <typename T>
    using FnGetPrivate = outcome::result<T> (CryptoStore::*)(
        KeyTypeId, const decltype(T::public_key) &) const;
    template <typename T,
              FnListPublic<T> list_public,
              FnGetPrivate<T> get_private,
              typename A,
              typename Eq>
    KeypairWithIndexOpt<T> find(KeypairWithIndexOpt<T> &cache,
                                KeyTypeId type,
                                const std::vector<A> &authorities,
                                const Eq &eq);

   public:
    SessionKeysImpl(std::shared_ptr<CryptoStore> store,
                    const application::AppConfiguration &config);

    KeypairWithIndexOpt<Sr25519Keypair> getBabeKeyPair(
        const primitives::AuthorityList &authorities) override;

    std::shared_ptr<Ed25519Keypair> getGranKeyPair(
        const primitives::AuthoritySet &authorities) override;

    KeypairWithIndexOpt<Sr25519Keypair> getParaKeyPair(
        const std::vector<Sr25519PublicKey> &authorities) override;

    std::shared_ptr<Sr25519Keypair> getAudiKeyPair(
        const std::vector<primitives::AuthorityDiscoveryId> &authorities)
        override;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_SESSION_KEYS_HPP
