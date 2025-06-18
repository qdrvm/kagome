/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "consensus/babe/types/authority.hpp"
// #include "consensus/sassafras/types/authority.hpp"
#include "consensus/grandpa/types/authority.hpp"
#include "crypto/ecdsa_types.hpp"
#include "crypto/key_store.hpp"
#include "crypto/key_store/key_type.hpp"
#include "network/types/roles.hpp"
#include "primitives/authority_discovery_id.hpp"

namespace kagome::application {
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::crypto {

  class KeyStore;
  struct Ed25519Keypair;
  struct Sr25519Keypair;
  struct Sr25519PublicKey;

  // hardcoded keys order for polkadot
  // otherwise it could be read from chainspec palletSession/keys
  // nevertheless they are hardcoded in polkadot
  // https://github.com/paritytech/polkadot/blob/634520cd3cf4b2b850db807daaaa32e480099981/node/service/src/chain_spec.rs#L230
  constexpr std::array<KeyType, 6> polkadot_key_order{
      KeyTypes::GRANDPA,
      KeyTypes::BABE,
      KeyTypes::IM_ONLINE,
      KeyTypes::PARACHAIN,
      KeyTypes::ASSIGNMENT,
      KeyTypes::AUTHORITY_DISCOVERY,
  };

  class SessionKeys {
   public:
    template <typename T, typename AuthorityIndexT = uint32_t>
    using KeypairWithIndexOpt =
        std::optional<std::pair<std::shared_ptr<T>, AuthorityIndexT>>;

    virtual ~SessionKeys() = default;

    /**
     * @return current BABE session key pair
     */
    virtual KeypairWithIndexOpt<Sr25519Keypair> getBabeKeyPair(
        const consensus::babe::Authorities &authorities) = 0;

    // /**
    //  * @return current SASSAFRAS session key pair
    //  */
    // virtual KeypairWithIndexOpt<BandersnatchKeypair> getSassafrasKeyPair(
    //     const consensus::sassafras::Authorities &authorities) = 0;

    /**
     * @return current GRANDPA session key pair
     */
    virtual std::shared_ptr<Ed25519Keypair> getGranKeyPair(
        const consensus::grandpa::AuthoritySet &authorities) = 0;

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

    /**
     * @return current AUDI session key pair from storage without checking
     * authority list
     * @note if there are multiple keys in storage, it returns the first one
     */
    virtual std::optional<Sr25519Keypair> getAudiKeyPair() = 0;

    /**
     * @return current BEEF session key pair
     */
    virtual KeypairWithIndexOpt<EcdsaKeypair> getBeefKeyPair(
        const std::vector<EcdsaPublicKey> &authorities) = 0;
  };

  class SessionKeysImpl : public SessionKeys {
    KeypairWithIndexOpt<Sr25519Keypair> babe_key_pair_;
    KeypairWithIndexOpt<Ed25519Keypair> gran_key_pair_;
    KeypairWithIndexOpt<Sr25519Keypair> para_key_pair_;
    KeypairWithIndexOpt<Sr25519Keypair> audi_key_pair_;
    KeypairWithIndexOpt<EcdsaKeypair> beef_key_pair_;
    network::Roles roles_;
    std::shared_ptr<KeyStore> store_;

    template <Suite T>
    using FnListPublic = outcome::result<std::vector<typename T::PublicKey>> (
        KeySuiteStore<T>::*)(KeyType) const;

    template <Suite T>
    using FnGetKeypair = outcome::result<typename T::Keypair> (
        KeySuiteStore<T>::*)(KeyType, const typename T::PublicKey &) const;

    template <Suite T, typename A, typename Eq>
    KeypairWithIndexOpt<typename T::Keypair> find(
        KeypairWithIndexOpt<typename T::Keypair> &cache,
        KeyType type,
        const KeySuiteStore<T> &store,
        const std::vector<A> &authorities,
        const Eq &eq);

   public:
    SessionKeysImpl(std::shared_ptr<KeyStore> store,
                    const application::AppConfiguration &config);

    KeypairWithIndexOpt<Sr25519Keypair> getBabeKeyPair(
        const consensus::babe::Authorities &authorities) override;

    // KeypairWithIndexOpt<BandersnatchKeypair> getSassafrasKeyPair(
    //     const consensus::sassafras::Authorities &authorities) override;

    std::shared_ptr<Ed25519Keypair> getGranKeyPair(
        const consensus::grandpa::AuthoritySet &authorities) override;

    KeypairWithIndexOpt<Sr25519Keypair> getParaKeyPair(
        const std::vector<Sr25519PublicKey> &authorities) override;

    std::shared_ptr<Sr25519Keypair> getAudiKeyPair(
        const std::vector<primitives::AuthorityDiscoveryId> &authorities)
        override;

    std::optional<Sr25519Keypair> getAudiKeyPair() override;

    KeypairWithIndexOpt<EcdsaKeypair> getBeefKeyPair(
        const std::vector<EcdsaPublicKey> &authorities) override;
  };

}  // namespace kagome::crypto
