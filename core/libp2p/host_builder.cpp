/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host_builder.hpp"

#include "libp2p/crypto/key_generator/key_generator_impl.hpp"
#include "libp2p/crypto/random_generator/boost_generator.hpp"
#include "libp2p/crypto/random_generator/std_generator.hpp"
#include "libp2p/dht/dht_impl.hpp"
#include "libp2p/discovery/discovery_impl.hpp"
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/peer/address_repository/inmem_address_repository.hpp"
#include "libp2p/peer/key_repository/inmem_key_repository.hpp"
#include "libp2p/peer/protocol_repository/inmem_protocol_repository.hpp"
#include "libp2p/routing/routing_impl.hpp"
#include "libp2p/security/security_impl.hpp"
#include "libp2p/transport/tcp.hpp"

namespace {
  /**
   * Check if both keys in the keypair have data and type set
   * @param keypair to be checked
   * @return true, if keypair is well-formed, false otherwise
   */
  bool keypairIsWellFormed(const libp2p::crypto::KeyPair &keypair) {
    using KeyType = libp2p::crypto::Key::Type;

    const auto &pubkey = keypair.publicKey;
    const auto &privkey = keypair.privateKey;
    return !pubkey.data.empty() && pubkey.type != KeyType::UNSPECIFIED
        && !privkey.data.empty() && privkey.type == pubkey.type;
  }
}  // namespace

namespace libp2p {
  using multi::Multiaddress;

  HostBuilder::HostBuilder(Config config) : config_{std::move(config)} {}

  HostBuilder &HostBuilder::setKeypair(const crypto::KeyPair &kp) {
    config_.peer_key = kp;
    return *this;
  }

  HostBuilder &HostBuilder::setCSPRNG(detail::sptr<crypto::random::CSPRNG> r) {
    config_.cprng = std::move(r);
    return *this;
  }

  HostBuilder &HostBuilder::setPRNG(
      detail::sptr<crypto::random::RandomGenerator> r) {
    config_.prng = std::move(r);
    return *this;
  }

  HostBuilder &HostBuilder::setRoutingAdaptor(
      detail::sptr<routing::RoutingAdaptor> r) {
    config_.routing = std::move(r);
    return *this;
  }

  HostBuilder &HostBuilder::setDiscoveryAdaptor(
      detail::sptr<discovery::DiscoveryAdaptor> d) {
    config_.discovery = std::move(d);
    return *this;
  }

  HostBuilder &HostBuilder::setPeerRepository(
      detail::sptr<peer::PeerRepository> p) {
    config_.peer_repository = std::move(p);
    return *this;
  }

  HostBuilder &HostBuilder::addTransport(
      detail::sptr<transport::Transport> tr) {
    config_.transports.push_back(std::move(tr));
    return *this;
  }

  HostBuilder &HostBuilder::addMuxerAdaptor(
      detail::sptr<muxer::MuxerAdaptor> mux) {
    config_.muxers.push_back(std::move(mux));
    return *this;
  }

  HostBuilder &HostBuilder::addDHTAdaptor(detail::sptr<dht::DHTAdaptor> d) {
    config_.dhts.push_back(std::move(d));
    return *this;
  }

  HostBuilder &HostBuilder::addSecurityAdaptor(
      detail::sptr<security::SecurityAdaptor> s) {
    config_.securities.push_back(std::move(s));
    return *this;
  }

  HostBuilder &HostBuilder::addListenMultiaddr(
      const multi::Multiaddress &address) {
    config_.listen_addresses.push_back(address);
    return *this;
  }

  HostBuilder &HostBuilder::addListenMultiaddr(std::string_view address) {
    multiaddr_candidates_.push_back(address);
    return *this;
  }

  HostBuilder &HostBuilder::setContext(
      std::shared_ptr<boost::asio::io_context> c) {
    config_.context = std::move(c);
    return *this;
  }

  outcome::result<Host> HostBuilder::build() {
    for (auto multiaddr_candidate : multiaddr_candidates_) {
      OUTCOME_TRY(addr, Multiaddress::create(multiaddr_candidate));
      config_.listen_addresses.push_back(std::move(addr));
    }

    if (!config_.cprng) {
      config_.cprng = std::make_shared<crypto::random::BoostRandomGenerator>();
    }

    if (!config_.prng) {
      config_.prng = std::make_shared<crypto::random::StdRandomGenerator>();
    }

    if (!keypairIsWellFormed(config_.peer_key)) {
      crypto::KeyGeneratorImpl key_generator{*config_.cprng};
      OUTCOME_TRY(keys, key_generator.generateKeys(crypto::Key::Type::RSA2048));
      config_.peer_key = std::move(keys);
    }

    OUTCOME_TRY(peer_id,
                peer::PeerId::fromPublicKey(config_.peer_key.publicKey));

    if (!config_.routing) {
      config_.routing = std::make_shared<routing::RoutingImpl>();
    }

    if (!config_.discovery) {
      config_.discovery = std::make_shared<discovery::DiscoveryImpl>();
    }

    if (!config_.peer_repository) {
      config_.peer_repository = std::make_shared<peer::PeerRepository>(
          std::make_shared<peer::InmemAddressRepository>(),
          std::make_shared<peer::InmemKeyRepository>(),
          std::make_shared<peer::InmemProtocolRepository>());
    }

    if (!config_.context) {
      config_.context = std::make_shared<boost::asio::io_context>(1);
    }

    if (!config_.executor) {
      config_.executor =
          std::make_shared<boost::asio::io_context::executor_type>(
              config_.context->get_executor());
    }

    if (config_.transports.empty()) {
      config_.transports.push_back(
          std::make_shared<
              transport::TcpTransport<boost::asio::io_context::executor_type>>(
              *config_.executor));
    }

    if (config_.muxers.empty()) {
      config_.muxers.push_back(std::make_shared<muxer::YamuxAdaptor>());
    }

    if (config_.dhts.empty()) {
      config_.dhts.push_back(std::make_shared<dht::DHTAdaptor>());
    }

    if (config_.securities.empty()) {
      config_.securities.push_back(std::make_shared<security::SecurityImpl>());
    }

    return Host{config_, std::move(peer_id)};
  }

}  // namespace libp2p
