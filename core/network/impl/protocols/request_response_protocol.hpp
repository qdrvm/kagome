/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/shared_fn.hpp>

#include "common/main_thread_pool.hpp"
#include "metrics/metrics.hpp"
#include "network/helpers/new_stream.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "utils/box.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::network {

  template <typename Request, typename Response>
  struct RequestResponseProtocol : virtual public ProtocolBase {
    using RequestType = Request;
    using ResponseType = Response;

    virtual void doRequest(const PeerId &peer_id,
                           RequestType request,
                           std::function<void(outcome::result<ResponseType>)>
                               &&response_handler) = 0;
  };

  struct RequestResponseInject {
    std::shared_ptr<libp2p::Host> host;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler;
    std::shared_ptr<common::MainThreadPool> main_thread_pool;
  };

  struct RequestResponseMetrics {
    RequestResponseMetrics(const std::string &name)
        : timeout_{metric(name, "timeout")},
          success_{metric(name, "success")},
          failure_{metric(name, "failure")},
          lost_{metric(name, "lost")} {}

    static metrics::Counter *metric(const std::string &protocol,
                                    const std::string &type) {
      auto name = "kagome_request_response_protocol_result";
      auto help =
          "Number of timeout, success, failure results for request response "
          "protocols.";
      static auto registry = [&] {
        auto registry = metrics::createRegistry();
        registry->registerCounterFamily(name, help);
        return registry;
      }();
      return registry->registerCounterMetric(
          name, {{"protocol", protocol}, {"type", type}});
    }

    class Lost {
     public:
      Lost(const Lost &) = delete;
      Lost(Lost &&other) : lost_{std::exchange(other.lost_, nullptr)} {}
      Lost(RequestResponseMetrics &metrics) : lost_{metrics.lost_} {}
      ~Lost() {
        if (lost_ != nullptr) {
          lost_->inc();
        }
      }
      void notLost() {
        lost_ = nullptr;
      }

     private:
      metrics::Counter *lost_;
    };

    metrics::Counter *timeout_;
    metrics::Counter *success_;
    metrics::Counter *failure_;
    metrics::Counter *lost_;
  };

  // TODO(turuslan): #2372, RequestResponseProtocol
  struct RequestResponseTimeout {
    template <typename T>
    static void wrap(const std::shared_ptr<T> &self,
                     auto &cb,
                     std::weak_ptr<Stream> weak_stream) {
      auto timer = self->scheduler_->scheduleWithHandle(
          [weak_self{std::weak_ptr{self}},
           weak_stream{std::move(weak_stream)}] {
            IF_WEAK_LOCK(stream) {
              stream->reset();
              IF_WEAK_LOCK(self) {
                self->metrics_.timeout_->inc();
              }
            }
          },
          self->timeout_);
      cb = libp2p::SharedFn{[weak_self{std::weak_ptr{self}},
                             cb{std::move(cb)},
                             lost{RequestResponseMetrics::Lost{self->metrics_}},
                             timer{std::move(timer)}](auto r) mutable {
        lost.notLost();
        timer.reset();
        IF_WEAK_LOCK(self) {
          (r ? self->metrics_.success_ : self->metrics_.failure_)->inc();
        }
        cb(std::move(r));
      }};
    }
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

    friend RequestResponseTimeout;

    RequestResponseProtocolImpl(Protocol name,
                                RequestResponseInject inject,
                                Protocols protocols,
                                log::Logger logger,
                                std::chrono::milliseconds timeout)
        : base_(name, *inject.host, std::move(protocols), std::move(logger)),
          metrics_{name},
          scheduler_{std::move(inject.scheduler)},
          timeout_(std::move(timeout)),
          main_pool_handler_{inject.main_thread_pool->handlerStarted()} {
      BOOST_ASSERT(scheduler_ != nullptr);
      BOOST_ASSERT(main_pool_handler_ != nullptr);
    }

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
          [WEAK_SELF,
           request{std::move(request)},
           response_handler{std::move(response_handler)}](auto &&res) mutable {
            if (res.has_error()) {
              response_handler(res.as_failure());
              return;
            }
            auto &stream = res.value();
            BOOST_ASSERT(stream);

            auto self = weak_self.lock();
            if (!self) {
              stream->reset();
              response_handler(outcome::failure(ProtocolError::GONE));
              return;
            }

            RequestResponseTimeout::wrap(self, response_handler, stream);

            SL_DEBUG(self->base_.logger(),
                     "Established outgoing {} stream with {}",
                     self->protocolName(),
                     stream->remotePeerId().value());

            self->writeRequest(std::move(stream),
                               std::move(request),
                               std::move(response_handler));
          });
    }

    void writeResponseAsync(std::shared_ptr<Stream> stream,
                            ResponseType response) {
      writeResponse(std::move(stream), std::move(response));
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
      newStream(
          base_.host(),
          peer_id,
          base_.protocolIds(),
          [wptr{this->weak_from_this()}, peer_id, cb{std::move(cb)}](
              libp2p::StreamAndProtocolOrError &&stream_and_proto) mutable {
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
          });
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
    RequestResponseMetrics metrics_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::chrono::milliseconds timeout_;

   private:
    std::shared_ptr<PoolHandler> main_pool_handler_;
  };

}  // namespace kagome::network
