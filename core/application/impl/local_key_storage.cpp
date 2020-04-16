/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "application/impl/local_key_storage.hpp"

#include <boost/property_tree/json_parser.hpp>
#include "application/impl/config_reader/error.hpp"
#include "application/impl/config_reader/pt_util.hpp"
#include "common/hexutil.hpp"

namespace kagome::application {

  outcome::result<std::shared_ptr<LocalKeyStorage>> LocalKeyStorage::create(
      const std::string &keystore_path) {
    auto storage = LocalKeyStorage();
    OUTCOME_TRY(storage.loadFromJson(keystore_path));
    return std::make_shared<LocalKeyStorage>(std::move(storage));
  }

  kagome::crypto::SR25519Keypair LocalKeyStorage::getLocalSr25519Keypair()
      const {
    return sr_25519_keypair_;
  }

  kagome::crypto::ED25519Keypair LocalKeyStorage::getLocalEd25519Keypair()
      const {
    return ed_25519_keypair_;
  }

  libp2p::crypto::KeyPair LocalKeyStorage::getP2PKeypair() const {
    return p2p_keypair_;
  }

  outcome::result<void> LocalKeyStorage::loadFromJson(const std::string &file) {
    namespace pt = boost::property_tree;
    pt::ptree tree;
    try {
      pt::read_json(file, tree);
    } catch (pt::json_parser_error &e) {
      return ConfigReaderError::PARSER_ERROR;
    }
    OUTCOME_TRY(loadSR25519Keys(tree));
    OUTCOME_TRY(loadED25519Keys(tree));
    OUTCOME_TRY(loadP2PKeys(tree));

    return outcome::success();
  }

  outcome::result<void> LocalKeyStorage::loadSR25519Keys(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(sr_tree, ensure(tree.get_child_optional("sr25519keypair")));

    OUTCOME_TRY(sr_pubkey_str,
                ensure(sr_tree.get_optional<std::string>("public")));
    OUTCOME_TRY(sr_privkey_str,
                ensure(sr_tree.get_optional<std::string>("private")));

    // get rid of 0x from beginning
    OUTCOME_TRY(sr_pubkey_buffer, common::unhexWith0x(sr_pubkey_str));
    OUTCOME_TRY(sr_privkey_buffer, common::unhexWith0x(sr_privkey_str));

    OUTCOME_TRY(sr_pubkey,
                crypto::SR25519PublicKey::fromSpan(sr_pubkey_buffer));
    OUTCOME_TRY(sr_privkey,
                crypto::SR25519SecretKey::fromSpan(sr_privkey_buffer));

    sr_25519_keypair_.public_key = sr_pubkey;
    sr_25519_keypair_.secret_key = sr_privkey;
    return outcome::success();
  }

  outcome::result<void> LocalKeyStorage::loadED25519Keys(
      const boost::property_tree::ptree &tree) {
    auto ed_tree_opt = tree.get_child_optional("ed25519keypair");
    if (not ed_tree_opt) return ConfigReaderError::MISSING_ENTRY;
    const auto &ed_tree = ed_tree_opt.value();

    OUTCOME_TRY(ed_pubkey_str,
                ensure(ed_tree.get_optional<std::string>("public")));
    OUTCOME_TRY(ed_privkey_str,
                ensure(ed_tree.get_optional<std::string>("private")));

    // get rid of 0x from beginning
    OUTCOME_TRY(ed_pubkey_buffer, common::unhexWith0x(ed_pubkey_str));
    OUTCOME_TRY(ed_privkey_buffer, common::unhexWith0x(ed_privkey_str));

    OUTCOME_TRY(ed_pubkey,
                crypto::ED25519PublicKey::fromSpan(ed_pubkey_buffer));
    OUTCOME_TRY(ed_privkey,
                crypto::ED25519PrivateKey::fromSpan(ed_privkey_buffer));

    ed_25519_keypair_.public_key = ed_pubkey;
    ed_25519_keypair_.private_key = ed_privkey;
    return outcome::success();
  }

  outcome::result<void> LocalKeyStorage::loadP2PKeys(
      const boost::property_tree::ptree &tree) {
    auto p2p_tree_opt = tree.get_child_optional("p2p_keypair");
    if (not p2p_tree_opt) return ConfigReaderError::MISSING_ENTRY;
    const auto &p2p_tree = p2p_tree_opt.value();

    OUTCOME_TRY(p2p_type,
                ensure(p2p_tree.get_optional<std::string>("p2p_type")));

    using KeyType = libp2p::crypto::Key::Type;
    if (p2p_type == "ed25519") {
      p2p_keypair_.publicKey.type = KeyType::Ed25519;
      p2p_keypair_.privateKey.type = KeyType::Ed25519;
    } else if (p2p_type == "rsa") {
      p2p_keypair_.publicKey.type = KeyType::RSA;
      p2p_keypair_.privateKey.type = KeyType::RSA;
    } else if (p2p_type == "secp256k1") {
      p2p_keypair_.publicKey.type = KeyType::Secp256k1;
      p2p_keypair_.privateKey.type = KeyType::Secp256k1;
    } else if (p2p_type == "ecdsa") {
      p2p_keypair_.publicKey.type = KeyType::ECDSA;
      p2p_keypair_.privateKey.type = KeyType::ECDSA;
    } else {
      p2p_keypair_.publicKey.type = KeyType::UNSPECIFIED;
      p2p_keypair_.privateKey.type = KeyType::UNSPECIFIED;
    }

    auto p2p_keypair_tree_opt = p2p_tree.get_child_optional("keypair");
    if (not p2p_keypair_tree_opt) return ConfigReaderError::MISSING_ENTRY;
    const auto &p2p_keypair_tree = p2p_keypair_tree_opt.value();

    OUTCOME_TRY(p2p_public_key_str,
                ensure(p2p_keypair_tree.get_optional<std::string>("public")));
    OUTCOME_TRY(p2p_private_key_str,
                ensure(p2p_keypair_tree.get_optional<std::string>("private")));

    // get rid of 0x from beginning
    OUTCOME_TRY(p2p_public_key, common::unhexWith0x(p2p_public_key_str));
    OUTCOME_TRY(p2p_private_key, common::unhexWith0x(p2p_private_key_str));

    p2p_keypair_.publicKey.data = p2p_public_key;
    p2p_keypair_.privateKey.data = p2p_private_key;

    return outcome::success();
  }
}  // namespace kagome::application
