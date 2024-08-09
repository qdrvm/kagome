/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/impl/protocols/protocol_base_impl.hpp"

#include "protocol_error.hpp"
#include "utils/box.hpp"

namespace kagome::network {

  template <typename Request, typename Response>
  struct RequestResponseProtocol : virtual public ProtocolBase {
    using RequestType = Request;
    using ResponseType = Response;

    virtual ~RequestResponseProtocol() = default;

    virtual void doRequest(const PeerId &peer_id,
                           RequestType request,
                           std::function<void(outcome::result<ResponseType>)>
                               &&response_handler) = 0;
  };

  template <typename Request, typename Response, typename ReadWriter>
  struct RequestResponseProtocolImpl
      : virtual protected ProtocolBase,
        virtual public RequestResponseProtocol<Request, Response>,
        std::enable_shared_from_this<
            RequestResponseProtocolImpl<Request, Response, ReadWriter>> {
    using RequestType = Request;
    using ResponseType = Response;
    using ReadWriterType = ReadWriter;

    RequestResponseProtocolImpl(
        Protocol name,
        libp2p::Host &host,
        Protocols protocols,
        log::Logger logger,
        std::chrono::milliseconds timeout = std::chrono::seconds(1))
        : base_(std::move(name), host, std::move(protocols), std::move(logger)),
          timeout_(std::move(timeout)) {}

    bool start() override {
      return base_.start(this->weak_from_this());
    }

    const ProtocolName &protocolName() const override {
      return base_.protocolName();
    }

    void doRequest(const PeerId &peer_id,
                   Request request,
                   std::function<void(outcome::result<Response>)>
                       &&response_handler) override {
      onTxRequest(request);
      newOutgoingStream(
          peer_id,
          [wptr{this->weak_from_this()},
           request{std::move(request)},
           response_handler{std::move(response_handler)}](auto &&res) mutable {
            if (res.has_error()) {
              response_handler(res.as_failure());
              return;
            }
            auto &stream = res.value();
            BOOST_ASSERT(stream);

            auto self = wptr.lock();
            if (!self) {
              self->base_.closeStream(std::move(wptr), std::move(stream));
              response_handler(outcome::failure(ProtocolError::GONE));
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
      BOOST_ASSERT(stream);
      BOOST_ASSERT(stream->remotePeerId().has_value());
      SL_DEBUG(base_.logger(),
               "New incoming {} stream with {}",
               protocolName(),
               stream->remotePeerId().value());
      readRequest(std::move(stream));
    }

    void newOutgoingStream(
        const PeerId &peer_id,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override {
      SL_TRACE(base_.logger(),
               "New outgoing {} stream with {}",
               protocolName(),
               peer_id);

      auto addresses_res =
          base_.host().getPeerRepository().getAddressRepository().getAddresses(
              peer_id);
      if (not addresses_res.has_value()) {
        cb(addresses_res.as_failure());
        return;
      }

      base_.host().newStream(
          PeerInfo{peer_id, std::move(addresses_res.value())},
          base_.protocolIds(),
          [wptr{this->weak_from_this()}, peer_id, cb{std::move(cb)}](
              auto &&stream_and_proto) mutable {
            if (!stream_and_proto.has_value()) {
              cb(stream_and_proto.as_failure());
              return;
            }

            auto &stream = stream_and_proto.value().stream;
            BOOST_ASSERT(stream);

            auto self = wptr.lock();
            if (not self) {
              cb(ProtocolError::GONE);
              self->base_.closeStream(std::move(wptr), std::move(stream));
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
      BOOST_ASSERT(stream);

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
            BOOST_ASSERT(stream);

            auto self = wptr.lock();
            if (not self) {
              cb(ProtocolError::GONE, nullptr);
              self->base_.closeStream(std::move(wptr), std::move(stream));
              return;
            }

            if (write_res.has_error()) {
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
      BOOST_ASSERT(stream);

      return write(
          std::move(stream),
          std::move(request),
          [wptr{this->weak_from_this()}, cb{std::move(cb)}](
              auto &&write_res, std::shared_ptr<Stream> stream) mutable {
            BOOST_ASSERT_MSG(write_res.has_value() ? stream != nullptr : true,
                             "Invariant is broken: "
                             "stream must not be null with success result");

            auto self = wptr.lock();
            if (!self) {
              cb(outcome::failure(ProtocolError::GONE));
              return;
            }

            if (write_res.has_error()) {
              cb(write_res.as_failure());
              return;
            }

            self->readResponse(std::move(stream), std::move(cb));
          });
    }

    void writeResponse(std::shared_ptr<Stream> stream, ResponseType response) {
      BOOST_ASSERT(stream);
      return write(
          std::move(stream),
          std::move(response),
          [wptr{this->weak_from_this()}](auto &&result,
                                         std::shared_ptr<Stream> stream) {
            BOOST_ASSERT_MSG(result.has_value() ? stream != nullptr : true,
                             "Invariant is broken: "
                             "stream must not be null with success result");

            if (result.has_value()) {
              auto self = wptr.lock();
              BOOST_ASSERT_MSG(
                  self, "If self not exists then we can not get result as OK.");

              BOOST_ASSERT(stream);
              self->base_.closeStream(std::move(wptr), std::move(stream));
            }
          });
    }

    template <typename M>
    void read(
        std::shared_ptr<Stream> stream,
        std::function<void(outcome::result<M>, std::shared_ptr<Stream>)> &&cb) {
      BOOST_ASSERT(stream);

      SL_DEBUG(base_.logger(),
               "Read from {} stream with {}",
               protocolName(),
               stream->remotePeerId().value());

      auto read_writer = std::make_shared<ReadWriterType>(stream);
      read_writer->template read<M>(
          [stream{std::move(stream)},
           wptr{this->weak_from_this()},
           cb{std::move(cb)}](outcome::result<M> read_result) mutable {
            BOOST_ASSERT(stream);

            auto self = wptr.lock();
            if (!self) {
              cb(outcome::failure(ProtocolError::GONE), nullptr);
              self->base_.closeStream(std::move(wptr), std::move(stream));
              return;
            }

            if (read_result.has_error()) {
              SL_DEBUG(self->base_.logger(),
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
            cb(outcome::success(std::move(read_result.value())),
               std::move(stream));
          });
    }

    void readResponse(std::shared_ptr<Stream> stream,
                      std::function<void(outcome::result<ResponseType>)> &&cb) {
      BOOST_ASSERT(stream);
      return read<ResponseType>(
          std::move(stream),
          [cb{std::move(cb)}, wptr{this->weak_from_this()}](
              auto &&result, std::shared_ptr<Stream> stream) {
            BOOST_ASSERT_MSG(result.has_value() ? stream != nullptr : true,
                             "Invariant is broken: "
                             "stream must not be null with success result");

            cb(std::move(result));
            if (result.has_value()) {
              auto self = wptr.lock();
              BOOST_ASSERT_MSG(
                  self, "If self not exists then we can not get result as OK.");

              self->base_.closeStream(std::move(wptr), std::move(stream));
            }
          });
    }

    void readRequest(std::shared_ptr<Stream> stream) {
      BOOST_ASSERT(stream);
      return read<RequestType>(
          std::move(stream),
          [wptr{this->weak_from_this()}](
              outcome::result<RequestType> request_res,
              std::shared_ptr<Stream> stream) {
            BOOST_ASSERT_MSG(request_res.has_value() ? stream != nullptr : true,
                             "Invariant is broken: "
                             "stream must not be null with success result");

            auto self = wptr.lock();
            BOOST_ASSERT_MSG(
                self, "If self not exists then we can not get result as OK.");

            if (request_res.has_error()) {
              SL_WARN(self->base_.logger(),
                      "Can't read incoming request from stream: {}",
                      request_res.error());
              return;
            }
            auto &request = request_res.value();

            auto response_opt = self->onRxRequest(std::move(request), stream);
            if (not response_opt) {
              // Request processing asynchronously
              return;
            }
            auto &response_result = response_opt.value();

            if (response_result.has_error()) {
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
