/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SYNCPROTOCOLIMPL
#define KAGOME_NETWORK_SYNCPROTOCOLIMPL

#include "network/protocols/sync_protocol.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <boost/circular_buffer.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/reputation_repository.hpp"
#include "network/sync_protocol_observer.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  static constexpr auto kResponsesCacheCapacity = 500;
  static constexpr auto kResponsesCacheExpirationTimeout =
      std::chrono::seconds(30);
  static constexpr auto kMaxCacheEntriesPerPeer = 5;

  namespace detail {

    /**
     * Container to store the most recent block requests by peers we replied to
     * with non empty responses.
     */
    class BlocksResponseCache {
     public:
      /**
       * Initialize the cache
       * @param capacity - max amount of cache entries
       * @param expiration_time - cache entry expiry time in seconds
       */
      BlocksResponseCache(std::size_t capacity,
                          std::chrono::seconds expiration_time);

      /**
       * Checks whether specified request came from the peer more than once.
       *
       * Some repeating request done past "expiration_time" seconds since last
       * request from the peer would not be considered as duplicating request.
       *
       * @param peer_id - peer identifier
       * @param request_fingerprint - request identifier
       * @return true when the request is found in a list of last peers'
       * requests of size kMaxCacheEntriesPerPeer and the last request
       * took a place not later than "expiration_time" seconds.
       * Otherwise - false.
       */
      bool isDuplicate(const PeerId &peer_id,
                       BlocksRequest::Fingerprint request_fingerprint);

     private:
      using ExpirationTimepoint =
          std::chrono::time_point<std::chrono::system_clock>;
      using CacheRecordIndex = std::size_t;

      /**
       * Save a record about peer's request to cache
       * @param peer_id - peer identifier
       * @param request_fingerprint - request identifier
       * @param target_slot - (optional) a slot of internal storage to put a
       * record into
       */
      void cache(const PeerId &peer_id,
                 BlocksRequest::Fingerprint request_fingerprint,
                 std::optional<CacheRecordIndex> target_slot = std::nullopt);

      /// removes all stale records
      void purge();

      struct CacheRecord {
        PeerId peer_id;
        ExpirationTimepoint valid_till;
        boost::circular_buffer<BlocksRequest::Fingerprint> fingerprints{
            kMaxCacheEntriesPerPeer};
      };

      const std::size_t capacity_;
      const std::chrono::seconds expiration_time_;
      std::unordered_map<PeerId, CacheRecordIndex> lookup_table_;
      std::vector<std::optional<CacheRecord>> storage_;
      std::unordered_set<CacheRecordIndex> free_slots_;
    };

  }  // namespace detail

  class SyncProtocolImpl final
      : public SyncProtocol,
        public std::enable_shared_from_this<SyncProtocolImpl>,
        NonCopyable,
        NonMovable {
   public:
    SyncProtocolImpl(
        libp2p::Host &host,
        const application::ChainSpec &chain_spec,
        const primitives::BlockHash &genesis_hash,
        std::shared_ptr<SyncProtocolObserver> sync_observer,
        std::shared_ptr<ReputationRepository> reputation_repository);

    bool start() override;

    const std::string &protocolName() const override {
      return kSyncProtocolName;
    }

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void request(const PeerId &peer_id,
                 BlocksRequest block_request,
                 std::function<void(outcome::result<BlocksResponse>)>
                     &&response_handler) override;

    void readRequest(std::shared_ptr<Stream> stream);

    void writeResponse(std::shared_ptr<Stream> stream,
                       const BlocksResponse &block_response);

    void writeRequest(std::shared_ptr<Stream> stream,
                      BlocksRequest block_request,
                      std::function<void(outcome::result<void>)> &&cb);

    void readResponse(std::shared_ptr<Stream> stream,
                      std::function<void(outcome::result<BlocksResponse>)>
                          &&response_handler);

   private:
    const static inline auto kSyncProtocolName = "SyncProtocol"s;
    ProtocolBaseImpl base_;
    std::shared_ptr<SyncProtocolObserver> sync_observer_;
    std::shared_ptr<ReputationRepository> reputation_repository_;
    detail::BlocksResponseCache response_cache_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SYNCPROTOCOLIMPL
