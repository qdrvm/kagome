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

#ifndef KAGOME_APPLICATION_KEY_STORAGE_HPP
#define KAGOME_APPLICATION_KEY_STORAGE_HPP

#include <libp2p/peer/peer_info.hpp>
#include <libp2p/crypto/key.hpp>
#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::application {

  /**
   * Stores crypto keys of the current node
   */
  class KeyStorage {
   public:
    virtual ~KeyStorage() = default;

    /**
     * Get the node sr25519 keypair, which is used, for example, in BABE
     */
    virtual crypto::SR25519Keypair getLocalSr25519Keypair() const = 0;

    /**
     * Get the node ed25519 keypair used in GRANDPA consensus
     */
    virtual crypto::ED25519Keypair getLocalEd25519Keypair() const = 0;

    /**
     * Get the node libp2p keypair, which is used by libp2p network library
     */
    virtual libp2p::crypto::KeyPair getP2PKeypair() const = 0;
  };

}

#endif  // KAGOME_APPLICATION_KEY_STORAGE_HPP
