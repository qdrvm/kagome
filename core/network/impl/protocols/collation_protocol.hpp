/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_COLLATIONPROTOCOL
#define KAGOME_NETWORK_COLLATIONPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/collation_observer.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/roles.hpp"
#include "network/types/status.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  class CollationProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<CollationProtocol>,
        NonCopyable,
        NonMovable {
   public:
    CollationProtocol() = delete;
    ~CollationProtocol() override = default;

    CollationProtocol(libp2p::Host &host,
                      application::AppConfiguration const &app_config,
                      application::ChainSpec const &chain_spec,
                      std::shared_ptr<CollationObserver> observer);

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    bool start() override;
    bool stop() override;

    const std::string &protocolName() const override {
      return kCollationProtocol;
    }

   private:
    template <bool DirectionIncoming, typename F>
    void exchangeHandshake(
        std::shared_ptr<kagome::network::Stream> const &stream, F &&func) {
      auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
      if constexpr (DirectionIncoming) {
        read_writer->read<Roles>([wptr{weak_from_this()},
                                  func{std::forward<F>(func)},
                                  stream](auto &&result) mutable {
          auto self = wptr.lock();
          if (!result || !self) return std::forward<F>(func)(std::move(result));

          auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
          read_writer->write(
              self->app_config_.roles(),
              [func{std::forward<F>(func)}, stream](auto &&result) mutable {
                return std::forward<F>(func)(std::move(result));
              });
        });
      } else {
        read_writer->write(
            app_config_.roles(),
            [wptr{weak_from_this()}, func{std::forward<F>(func)}, stream](
                auto &&result) mutable {
              auto self = wptr.lock();
              if (!result || !self)
                return std::forward<F>(func)(std::move(result));

              auto read_writer =
                  std::make_shared<ScaleMessageReadWriter>(stream);
              read_writer->read<Roles>(
                  [func{std::forward<F>(func)}, stream](auto &&result) mutable {
                    return std::forward<F>(func)(std::move(result));
                  });
            });
      }
    }

    template <bool DirectionIncoming, typename F>
    void doCollatorHandshake(
        std::shared_ptr<kagome::network::Stream> const &stream, F &&func) {
      exchangeHandshake<DirectionIncoming>(
          stream,
          [func{std::forward<F>(func)}, stream, wptr{weak_from_this()}](
              auto &&result) mutable {
            if (auto self = wptr.lock()) {
              if (!result) {
                SL_WARN(self->base_.logger(),
                        "Handshake with {} failed with error {}",
                        stream->remotePeerId().value(),
                        result.error().message());
                self->base_.closeStream(wptr, stream);
                std::forward<F>(func)(nullptr);
                return;
              }
            }
            std::forward<F>(func)(stream);
          });
    }

    void readCollationMsg(std::shared_ptr<kagome::network::Stream> stream);

    void onCollationMessageRx(libp2p::peer::PeerId const &peer_id,
                              CollationMessage &&collation_message);
    void onCollationDeclRx(libp2p::peer::PeerId const &peer_id,
                           CollatorDeclaration &&collation_decl);
    void onCollationAdvRx(libp2p::peer::PeerId const &peer_id,
                          CollatorAdvertisement &&collation_adv);

    ProtocolBaseImpl base_;
    std::shared_ptr<CollationObserver> observer_;
    application::AppConfiguration const &app_config_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_COLLATIONPROTOCOL
