/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_REQUESTRESPONSEPROTOCOL
#define KAGOME_NETWORK_REQUESTRESPONSEPROTOCOL

#include "network/impl/protocols/protocol_base_impl.hpp"

#include "network/helpers/peer_id_formatter.hpp"
#include "protocol_error.hpp"
#include "utils/box.hpp"

namespace kagome::network {

  template <typename Request, typename Response, typename ReadWriter>
  struct RequestResponseProtocol
      : ProtocolBase,
        std::enable_shared_from_this<
            RequestResponseProtocol<Request, Response, ReadWriter>> {
    using RequestResponseProtocolType =
        RequestResponseProtocol<Request, Response, ReadWriter>;
    using RequestType = Request;
    using ResponseType = Response;
    using ReadWriterType = ReadWriter;

    RequestResponseProtocol(
        Protocol name,
        libp2p::Host &host,
        Protocols protocols,
        log::Logger logger,
        std::chrono::milliseconds timeout = std::chrono::seconds(1))
        : base_(std::move(name), host, std::move(protocols), std::move(logger)),
          timeout_(std::move(timeout)) {}
    virtual ~RequestResponseProtocol() {}

    bool start() override {
      return base_.start(this->weak_from_this());
    }

    const ProtocolName &protocolName() const override {
      return base_.protocolName();
    }

    void doRequest(
        const PeerId &peer_id,
        RequestType request,
        std::function<void(outcome::result<ResponseType>)> &&response_handler) {
      doRequest({peer_id, {}}, std::move(request), std::move(response_handler));
    }

    void doRequest(
        const PeerInfo &peer_info,
        RequestType request,
        std::function<void(outcome::result<ResponseType>)> &&response_handler) {
      onTxRequest(request);
      newOutgoingStream(
          peer_info,
          [wptr{this->weak_from_this()},
           response_handler{std::move(response_handler)},
           request{std::move(request)}](auto &&stream_res) mutable {
            if (!stream_res.has_value()) {
              response_handler(stream_res.as_failure());
              return;
            }

            auto stream = std::move(stream_res.value());
            auto self = wptr.lock();
            if (!self) {
              stream->close([stream](auto &&) {});
              response_handler(ProtocolError::GONE);
              return;
            }

            SL_DEBUG(self->base_.logger(),
                     "Established outgoing {} stream with {}",
                     self->protocolName(),
                     stream->remotePeerId().value());

            self->writeRequest(std::move(stream),
                               std::move(request),
                               std::move(response_handler));
          });
    }

   protected:
    virtual std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType request, std::shared_ptr<Stream> stream) = 0;
    virtual void onTxRequest(const RequestType &request) = 0;

    ProtocolBaseImpl &base() {
      return base_;
    }

    friend class ProtocolBaseImpl;

    void onIncomingStream(std::shared_ptr<Stream> stream) override {
      BOOST_ASSERT(stream->remotePeerId().has_value());
      SL_INFO(base_.logger(),
              "New incoming {} stream with {}",
              protocolName(),
              stream->remotePeerId().value());
      readRequest(std::move(stream));
    }

    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override {
      SL_TRACE(base_.logger(),
               "Connect for {} stream with {}",
               protocolName(),
               peer_info.id);

      base_.host().newStream(
          peer_info,
          base_.protocolIds(),
          [wptr{this->weak_from_this()},
           peer_id{peer_info.id},
           cb{std::move(cb)}](auto &&stream_and_proto) mutable {
            if (!stream_and_proto.has_value()) {
              cb(stream_and_proto.as_failure());
              return;
            }

            auto stream = std::move(stream_and_proto.value().stream);
            auto self = wptr.lock();
            if (!self) {
              cb(ProtocolError::GONE);
              stream->close([stream](auto &&) {});
              return;
            }

            SL_DEBUG(self->base_.logger(),
                     "Established connection over {} stream with {}",
                     self->protocolName(),
                     peer_id);
            cb(std::move(stream));
          },
          timeout_);
    }

    template <typename M>
    void write(std::shared_ptr<Stream> stream,
               M msg,
               std::function<void(outcome::result<void>,
                                  std::shared_ptr<Stream>)> &&cb) {
      static_assert(std::is_same_v<M, RequestType>
                    || std::is_same_v<M, ResponseType>);
      SL_DEBUG(base_.logger(),
               "Write msg into {} stream with {}",
               protocolName(),
               stream->remotePeerId().value());

      auto read_writer = std::make_shared<ReadWriterType>(stream);
      read_writer->write(
          msg,
          [stream{std::move(stream)},
           wptr{this->weak_from_this()},
           cb{std::move(cb)}](auto &&write_res) mutable {
            auto self = wptr.lock();
            if (not self) {
              cb(ProtocolError::GONE, nullptr);
              stream->close([stream](auto &&) {});
              return;
            }

            if (!write_res.has_value()) {
              SL_VERBOSE(self->base_.logger(),
                         "Error at write into {} stream with {}: {}",
                         self->protocolName(),
                         stream->remotePeerId().value(),
                         write_res.error());

              cb(write_res.as_failure(), nullptr);
              self->base_.closeStream(std::move(wptr), std::move(stream));
              return;
            }

            SL_DEBUG(
                self->base_.logger(),
                "Request written successful into outgoing {} stream with {}",
                self->protocolName(),
                stream->remotePeerId().value());
            cb(outcome::success(), std::move(stream));
          });
    }

    void writeRequest(std::shared_ptr<Stream> stream,
                      RequestType request,
                      std::function<void(outcome::result<ResponseType>)> &&cb) {
      return write(
          std::move(stream),
          std::move(request),
          [wptr{this->weak_from_this()}, cb{std::move(cb)}](
              auto &&write_res, std::shared_ptr<Stream> stream) mutable {
            auto self = wptr.lock();
            if (!self) {
              cb(ProtocolError::GONE);
              return;
            }
            if (!write_res.has_value()) {
              cb(write_res.as_failure());
              return;
            }
            self->readResponse(std::move(stream), std::move(cb));
          });
    }

    void writeResponse(std::shared_ptr<Stream> stream, ResponseType response) {
      return write(
          std::move(stream),
          std::move(response),
          [wptr{this->weak_from_this()}](auto &&result,
                                         std::shared_ptr<Stream> stream) {
            if (result) {
              auto self = wptr.lock();
              BOOST_ASSERT_MSG(
                  self, "If self not exists then we can not get result as OK.");
              self->base_.closeStream(std::move(wptr), std::move(stream));
            }
          });
    }

    template <typename M>
    void read(
        std::shared_ptr<Stream> stream,
        std::function<void(outcome::result<M>, std::shared_ptr<Stream>)> &&cb) {
      SL_DEBUG(base_.logger(),
               "Read from {} stream with {}",
               protocolName(),
               stream->remotePeerId().value());

      auto read_writer = std::make_shared<ReadWriterType>(stream);
      read_writer->template read<M>(
          [stream{std::move(stream)},
           wptr{this->weak_from_this()},
           cb{std::move(cb)}](auto &&read_result) mutable {
            auto self = wptr.lock();
            if (!self) {
              cb(ProtocolError::GONE, nullptr);
              stream->close([stream](auto &&) {});
              return;
            }

            if (!read_result.has_value()) {
              SL_VERBOSE(self->base_.logger(),
                         "Error at read from outgoing {} stream with {}: {}",
                         self->protocolName(),
                         stream->remotePeerId().value(),
                         read_result.error());

              cb(read_result.as_failure(), nullptr);
              self->base_.closeStream(std::move(wptr), std::move(stream));
              return;
            }

            SL_DEBUG(self->base_.logger(),
                     "Successful response read from outgoing {} stream with {}",
                     self->protocolName(),
                     stream->remotePeerId().value());
            cb(std::move(read_result.value()), std::move(stream));
          });
    }

    void readResponse(std::shared_ptr<Stream> stream,
                      std::function<void(outcome::result<ResponseType>)> &&cb) {
      return read<ResponseType>(
          std::move(stream),
          [cb{std::move(cb)}, wptr{this->weak_from_this()}](
              auto &&result, std::shared_ptr<Stream> stream) {
            cb(result);
            if (result) {
              auto self = wptr.lock();
              assert(
                  self
                  && !!"If self not exists then we can not get result as OK.");
              self->base_.closeStream(std::move(wptr), std::move(stream));
            }
          });
    }

    void readRequest(std::shared_ptr<Stream> stream) {
      return read<RequestType>(
          std::move(stream),
          [wptr{this->weak_from_this()}](outcome::result<RequestType> result,
                                         std::shared_ptr<Stream> stream) {
            if (!result) {
              return;
            }

            auto self = wptr.lock();
            assert(self
                   && !!"If self not exists then we can not get result as OK.");

            auto request = std::move(result.value());
            auto response_opt = self->onRxRequest(request, stream);
            if (not response_opt) {
              // Request processing asynchronously
              return;
            }
            auto &response_result = response_opt.value();

            if (!response_result) {
              SL_VERBOSE(self->base_.logger(),
                         "Error at execute request from incoming {} stream "
                         "with {}: {}",
                         self->protocolName(),
                         stream->remotePeerId().value(),
                         response_result.error());
              self->base_.closeStream(std::move(wptr), std::move(stream));
              return;
            }
            self->writeResponse(std::move(stream),
                                std::move(response_result.value()));
          });
    }

    ProtocolBaseImpl base_;
    std::chrono::milliseconds timeout_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_REQUESTRESPONSEPROTOCOL
