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

#ifndef KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP
#define KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP

#include "application/key_storage.hpp"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

#include "common/buffer.hpp"

namespace kagome::application {

  class LocalKeyStorage : public KeyStorage {
   public:
    static outcome::result<std::shared_ptr<LocalKeyStorage>> create(
        const std::string &keystore_path);

    ~LocalKeyStorage() override = default;

    crypto::SR25519Keypair getLocalSr25519Keypair() const override;
    crypto::ED25519Keypair getLocalEd25519Keypair() const override;
    libp2p::crypto::KeyPair getP2PKeypair() const override;

   private:
    LocalKeyStorage() = default;

    /**
     * Loads a keystore from the provided file
     */
    outcome::result<void> loadFromJson(const std::string &file_path);

    outcome::result<void> loadSR25519Keys(
        const boost::property_tree::ptree &tree);
    outcome::result<void> loadED25519Keys(
        const boost::property_tree::ptree &tree);
    outcome::result<void> loadP2PKeys(const boost::property_tree::ptree &tree);

    crypto::SR25519Keypair sr_25519_keypair_;
    crypto::ED25519Keypair ed_25519_keypair_;
    libp2p::crypto::KeyPair p2p_keypair_;
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP
