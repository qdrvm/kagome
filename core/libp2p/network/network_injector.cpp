/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/network_injector.hpp"

// implementations
#include "libp2p/crypto/key_generator/key_generator_impl.hpp"
#include "libp2p/crypto/marshaller/key_marshaller_impl.hpp"
#include "libp2p/crypto/random_generator/boost_generator.hpp"
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/network/impl/connection_manager_impl.hpp"
#include "libp2p/network/impl/dialer_impl.hpp"
#include "libp2p/network/impl/listener_manager_impl.hpp"
#include "libp2p/network/impl/network_impl.hpp"
#include "libp2p/network/impl/router_impl.hpp"
#include "libp2p/network/impl/transport_manager_impl.hpp"
#include "libp2p/peer/impl/identity_manager_impl.hpp"
#include "libp2p/protocol_muxer/multiselect.hpp"
#include "libp2p/security/plaintext.hpp"
#include "libp2p/transport/impl/upgrader_impl.hpp"
#include "libp2p/transport/tcp.hpp"

namespace libp2p::network::injector {

  auto makeIdentity(crypto::KeyPair keyPair) {
    using namespace boost;  // NOLINT

    // clang-format off
    return di::make_injector(
        di::bind<crypto::marshaller::KeyMarshaller>.to<crypto::marshaller::KeyMarshallerImpl>(),
        di::bind<peer::IdentityManager>.to<peer::IdentityManagerImpl>(),
        di::bind<crypto::KeyPair>.to(std::move(keyPair))
    );
    // clang-format on
  }

  auto makeRandomIdentity() {
    using namespace boost;  // NOLINT

    auto csprng = std::make_shared<crypto::random::BoostRandomGenerator>();
    auto gen = std::make_shared<crypto::KeyGeneratorImpl>(*csprng);

    // assume no error here. otherwise... just blow up executable
    auto keypair = gen->generateKeys(crypto::Key::Type::ED25519).value();

    // clang-format off
    return di::make_injector(
        di::bind<crypto::random::CSPRNG>.to(std::move(csprng)),
        di::bind<crypto::KeyGenerator>.to(std::move(gen)),
        makeIdentity(std::move(keypair))
    );
    // clang-format on
  }

  boost::di::injector<std::shared_ptr<Network>> makeDefaultNetwork() {
    using namespace boost;

    // clang-format off
    return boost::di::make_injector(
      // identity
      makeRandomIdentity(),

      // internal
      di::bind<network::Router>.to<network::RouterImpl>(),
      di::bind<network::Network>.to<network::NetworkImpl>(),
      di::bind<network::ConnectionManager>.to<network::ConnectionManagerImpl>(),
      di::bind<network::TransportManager>.to<network::TransportManagerImpl>(),
      di::bind<network::ListenerManager>.to<network::ListenerManagerImpl>(),
      di::bind<network::Dialer>.to<network::DialerImpl>(),
      di::bind<transport::Upgrader>.to<transport::UpgraderImpl>(),
      di::bind<protocol_muxer::ProtocolMuxer>.to<protocol_muxer::Multiselect>()

//      // adaptors
//      di::bind<transport::TransportAdaptor *[]>.to<transport::TcpTransport>(),
//      di::bind<muxer::MuxerAdaptor *[]>.to<muxer::Yamux>(),
//      di::bind<security::SecurityAdaptor *[]>.to<security::Plaintext>()
    );
    // clang-format on
  }

}  // namespace libp2p::network::injector
