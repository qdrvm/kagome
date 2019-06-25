#include <utility>

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/security/plaintext.hpp"
#include "libp2p/transport/tcp.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace transport;
using namespace connection;
using namespace muxer;
using namespace security;
using namespace multi;
using namespace peer;

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using std::chrono_literals::operator""ms;

static const size_t kServerBufSize = 10000;  // 10 Kb

ACTION_P(Upgrade, do_upgrade) {
  // arg0 - prev conn
  // arg1 - callback(next conn)
  arg1(do_upgrade(arg0));
}

struct Server : public std::enable_shared_from_this<Server> {
  explicit Server(std::shared_ptr<MuxerAdaptor> muxer,
                  boost::asio::io_context &context)
      : muxer_adaptor_(std::move(muxer)) {
    upgrader_ = std::make_shared<NiceMock<UpgraderMock>>();

    EXPECT_CALL(*upgrader_, upgradeToSecure(_, _))
        .WillRepeatedly(Upgrade([&](std::shared_ptr<RawConnection> raw) {
          println("secure inbound");
          return this->security_adaptor_->secureInbound(std::move(raw));
        }));
    EXPECT_CALL(*upgrader_, upgradeToMuxed(_, _))
        .WillRepeatedly(Upgrade([&](std::shared_ptr<SecureConnection> sec) {
          println("mux connection");
          auto c = this->muxer_adaptor_->muxConnection(std::move(sec)).value();
          c->start();
          return c;
        }));
    Mock::AllowLeak(upgrader_.get());

    transport_ = std::make_shared<TcpTransport>(context, upgrader_);

    listener_ = transport_->createListener(
        [this](outcome::result<std::shared_ptr<CapableConnection>> rconn) {
          EXPECT_OUTCOME_TRUE(conn, rconn)
          this->println("new connection received");
          this->onConnection(conn);
        });
  }

  void onConnection(const std::shared_ptr<CapableConnection> &conn) {
    this->clientsConnected++;

    conn->onStream([conn, self{shared_from_this()}](
                       outcome::result<std::shared_ptr<Stream>> rstream) {
      EXPECT_OUTCOME_TRUE(stream, rstream)
      self->println("new stream created");
      self->streamsCreated++;
      self->onStream(stream);
    });
  }

  void onStream(const std::shared_ptr<Stream> &stream) {
    // we should create buffer per stream (session)
    auto buf = std::make_shared<std::vector<uint8_t>>();
    buf->resize(kServerBufSize);

    println("onStream executed");

    stream->readSome(*buf, buf->size(),
                     [buf, stream, self{this->shared_from_this()}](
                         outcome::result<size_t> rread) {
                       EXPECT_OUTCOME_TRUE(read, rread)

                       self->println("readSome ", read, " bytes");
                       self->streamReads++;

                       // echo back read data
                       stream->write(*buf, read,
                                     [buf, read, stream,
                                      self](outcome::result<size_t> rwrite) {
                                       EXPECT_OUTCOME_TRUE(write, rwrite)
                                       self->println("write ", write, " bytes");
                                       self->streamWrites++;
                                       ASSERT_EQ(write, read);

                                       self->onStream(stream);
                                     });
                     });
  }

  void listen(const Multiaddress &ma){
      EXPECT_OUTCOME_TRUE_1(this->listener_->listen(ma))}

  size_t clientsConnected = 0;
  size_t streamsCreated = 0;
  size_t streamReads = 0;
  size_t streamWrites = 0;

 private:
  template <typename... Args>
  void println(Args &&... args) {
    std::cout << "[server " << std::this_thread::get_id() << "]\t";
    (std::cout << ... << args);
    std::cout << std::endl;
  }

  std::shared_ptr<UpgraderMock> upgrader_;
  std::shared_ptr<Transport> transport_;

  std::shared_ptr<TransportListener> listener_;

  std::shared_ptr<SecurityAdaptor> security_adaptor_ =
      std::make_shared<Plaintext>();
  std::shared_ptr<MuxerAdaptor> muxer_adaptor_;
};

struct Client : public std::enable_shared_from_this<Client> {
  Client(std::shared_ptr<MuxerAdaptor> muxer, size_t seed,
         boost::asio::io_context &context, PeerId p, size_t streams,
         size_t rounds)
      : context_(context),
        peer_id_(std::move(p)),
        streams_(streams),
        rounds_(rounds),
        generator(seed),
        distribution(1, kServerBufSize),
        muxer_adaptor_(std::move(muxer)) {
    upgrader_ = std::make_shared<UpgraderMock>();

    EXPECT_CALL(*upgrader_, upgradeToSecure(_, _))
        .WillRepeatedly(Upgrade([&](std::shared_ptr<RawConnection> raw) {
          println("secure outbound");
          return this->security_adaptor_->secureOutbound(raw, this->peer_id_);
        }));
    EXPECT_CALL(*upgrader_, upgradeToMuxed(_, _))
        .WillRepeatedly(Upgrade([&](std::shared_ptr<SecureConnection> sec) {
          // client has its own muxer, therefore, upgrader
          println("mux connection");
          auto c = this->muxer_adaptor_->muxConnection(sec).value();
          c->start();
          return c;
        }));
    Mock::AllowLeak(upgrader_.get());

    transport_ = std::make_shared<TcpTransport>(context, upgrader_);
  }

  void connect(const Multiaddress &server) {
    // create new stream
    transport_->dial(
        server,
        [self{this->shared_from_this()}](
            outcome::result<std::shared_ptr<CapableConnection>> rconn) {
          EXPECT_OUTCOME_TRUE(conn, rconn);
          self->println("connected");
          self->onConnection(conn);
        });
  }

  void onConnection(const std::shared_ptr<CapableConnection> &conn) {
    for (size_t i = 0; i < streams_; i++) {
      boost::asio::post(context_, [i, conn, self{this->shared_from_this()}]() {
        conn->newStream(
            [i, conn, self](outcome::result<std::shared_ptr<Stream>> rstream) {
              EXPECT_OUTCOME_TRUE(stream, rstream);
              self->println("new stream number ", i, " created");
              self->onStream(i, self->rounds_, stream);
            });
      });
    }
  }

  void onStream(size_t streamId, size_t round,
                const std::shared_ptr<Stream> &stream) {
    this->println(streamId, " onStream round ", round);
    if (round <= 0) {
      return;
    }

    auto buf = randomBuffer();
    stream->write(
        *buf, buf->size(),
        [round, streamId, buf, stream,
         self{this->shared_from_this()}](outcome::result<size_t> rwrite) {
          EXPECT_OUTCOME_TRUE(write, rwrite);
          self->println(streamId, " write ", write, " bytes");
          self->streamWrites++;

          auto readbuf = std::make_shared<std::vector<uint8_t>>();
          readbuf->resize(write);

          stream->readSome(*readbuf, readbuf->size(),
                           [round, streamId, write, buf, readbuf, stream,
                            self](outcome::result<size_t> rread) {
                             EXPECT_OUTCOME_TRUE(read, rread);
                             self->println(streamId, " readSome ", read,
                                           " bytes");
                             self->streamReads++;

                             ASSERT_EQ(write, read);
                             ASSERT_EQ(*buf, *readbuf);

                             self->onStream(streamId, round - 1, stream);
                           });
        });
  }

  size_t streamWrites = 0;
  size_t streamReads = 0;

 private:
  template <typename... Args>
  void println(Args &&... args) {
    std::cout << "[client " << std::this_thread::get_id() << "]\t";
    (std::cout << ... << args);
    std::cout << std::endl;
  }

  size_t rand() {
    return distribution(generator);
  }

  std::shared_ptr<std::vector<uint8_t>> randomBuffer() {
    auto buf = std::make_shared<std::vector<uint8_t>>();
    buf->resize(this->rand());
    this->println("random buffer of size ", buf->size(), " generated");
    std::generate(buf->begin(), buf->end(), [self{this->shared_from_this()}]() {
      return self->rand() & 0xff;
    });
    return buf;
  }

  boost::asio::io_context &context_;

  PeerId peer_id_;

  size_t streams_;
  size_t rounds_;

  std::default_random_engine generator;
  std::uniform_int_distribution<int> distribution;

  std::shared_ptr<UpgraderMock> upgrader_;
  std::shared_ptr<Transport> transport_;

  std::shared_ptr<SecurityAdaptor> security_adaptor_ =
      std::make_shared<Plaintext>();
  std::shared_ptr<MuxerAdaptor> muxer_adaptor_;
};

struct MuxerAcceptanceTest
    : public ::testing::TestWithParam<std::shared_ptr<MuxerAdaptor>> {
  struct PrintToStringParamName {
    template <class ParamType>
    std::string operator()(
        const testing::TestParamInfo<ParamType> &info) const {
      auto m = static_cast<std::shared_ptr<MuxerAdaptor>>(info.param);
      auto p = m->getProtocolId();

      return p.substr(1, p.find('/', 1) - 1);
    }
  };
};

TEST_P(MuxerAcceptanceTest, ParallelEcho) {
  // total number of parallel clients
  const int totalClients = 3;
  // total number of streams per connection
  const int streams = 10;
  // total number of rounds per stream
  const int rounds = 10;
  // number, which makes tests reproducible
  const int seed = 0;

  boost::asio::io_context context(1);
  std::default_random_engine randomEngine(seed);

  auto serverAddr = "/ip4/127.0.0.1/tcp/40312"_multiaddr;

  auto muxer = GetParam();
  auto server = std::make_shared<Server>(muxer, context);
  server->listen(serverAddr);

  std::vector<std::thread> clients;
  clients.reserve(totalClients);
  for (int i = 0; i < totalClients; i++) {
    clients.emplace_back([&]() {
      boost::asio::io_context context(1);
      auto pid = testutil::randomPeerId();

      auto localSeed = randomEngine();
      auto muxer = GetParam();
      auto client = std::make_shared<Client>(muxer, localSeed, context, pid,
                                             streams, rounds);
      client->connect(serverAddr);

      context.run_for(2000ms);

      EXPECT_EQ(client->streamWrites, rounds * streams);
      EXPECT_EQ(client->streamReads, rounds * streams);
    });
  }

  context.run_for(3000ms);

  for (auto &c : clients) {
    if (c.joinable()) {
      c.join();
    }
  }

  EXPECT_EQ(server->clientsConnected, totalClients);
  EXPECT_EQ(server->streamsCreated, totalClients * streams);
  EXPECT_EQ(server->streamReads, totalClients * streams * rounds);
  EXPECT_EQ(server->streamWrites, totalClients * streams * rounds);
}

INSTANTIATE_TEST_CASE_P(AllMuxers, MuxerAcceptanceTest,
                        ::testing::Values(
                            // list here all muxers
                            std::make_shared<Yamux>()),
                        MuxerAcceptanceTest::PrintToStringParamName());
