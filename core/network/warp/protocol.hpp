/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/warp/cache.hpp"

#include <random>

namespace kagome::network {

  using WarpRequest = primitives::BlockHash;
  using WarpResponse = WarpSyncProof;

  class WarpProtocol
      : virtual public RequestResponseProtocol<WarpRequest, WarpResponse> {
   public:
    using Cb = std::function<void(outcome::result<ResponseType>)>;
    virtual void random(WarpRequest req, Cb cb) = 0;
  };

  class WarpProtocolImpl
      : public WarpProtocol,
        public RequestResponseProtocolImpl<WarpRequest,
                                           WarpResponse,
                                           ScaleMessageReadWriter> {
    static constexpr auto kName = "WarpProtocol";

   public:
    WarpProtocolImpl(libp2p::Host &host,
                     const application::ChainSpec &chain_spec,
                     const blockchain::GenesisBlockHash &genesis,
                     std::shared_ptr<WarpSyncCache> cache,
                     common::MainThreadPool &main_thread_pool)
        : RequestResponseProtocolImpl(
              kName,
              host,
              make_protocols(kWarpProtocol, genesis, chain_spec),
              log::createLogger(kName, "warp_sync_protocol"),
              main_thread_pool),
          cache_{std::move(cache)} {}

    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType after_hash, std::shared_ptr<Stream>) override {
      return cache_->getProof(after_hash);
    }

    void onTxRequest(const RequestType &) override {}

@@ -49,56 +49,88 @@
+    /**
+     * @brief Sends a request to a randomly selected peer.
+     *
+     * This method selects a random peer that supports the necessary protocols and sends a `WarpRequest` to it. 
+     * If no suitable peer is found, it tries to use any connected peer.
+     *
+     * @param req The WarpRequest to be sent.
+     * @param cb The callback function to handle the response.
+     */
+    void random(WarpRequest req, Cb cb) override {
+      // Vector to store the peers that support the required protocols
+      std::vector<PeerId> peers;
+      
+      // Get the list of protocol IDs
+      auto &protocols1 = base().protocolIds();
+      // Convert protocol IDs to a set for easier comparison
+      std::set protocols2(protocols1.begin(), protocols1.end());
+      
+      // Get references to the protocol repository and connection manager
+      auto &protocol_repo = base().host().getPeerRepository().getProtocolRepository();
+      auto &conns = base().host().getNetwork().getConnectionManager();
+      
+      // Loop through all peers in the protocol repository
+      for (auto &peer : protocol_repo.getPeers()) {
+        // Skip peers without an active connection
+        if (not conns.getBestConnectionForPeer(peer)) {
+          continue;
+        }
+        
+        // Check if the peer supports the required protocols
+        auto common = protocol_repo.supportsProtocols(peer, protocols2);
+        if (not common) {
+          continue;
+        }
+        
+        // Skip peers that don't support any of the required protocols
+        if (common.value().empty()) {
+          continue;
+        }
+        
+        // Add the peer to the list of suitable peers
+        peers.emplace_back(peer);
+      }
+      
+      // If no suitable peers were found, add all connected peers to the list
+      if (peers.empty()) {
+        for (auto &conn : conns.getConnections()) {
+          if (auto peer = conn->remotePeer()) {
+            peers.emplace_back(peer.value());
+          }
+        }
+      }
+      
+      // If there are still no peers, return without sending the request
+      if (peers.empty()) {
+        return;
+      }
+      
+      // Select a random peer from the list
+      auto peer = peers[random_() % peers.size()];
+      
+      // Send the request to the selected peer
+      doRequest(peer, req, std::move(cb));
+    }

   private:
    std::shared_ptr<WarpSyncCache> cache_;
    std::default_random_engine random_;
  };
}  // namespace kagome::network
