#include <backward.hpp>
#undef TRUE
#undef FALSE

#include <boost/di.hpp>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/injector/kademlia_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/identify.hpp>
#include <libp2p/protocol/ping.hpp>
#include <libp2p/protocol/ping/common.hpp>
#include <libp2p/protocol/ping/ping_config.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>

#include "application/impl/app_state_manager_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "utils/grandpa_protocol.hpp"

namespace di = boost::di;
using namespace kagome;

template <class T>
using sptr = std::shared_ptr<T>;

template <typename C>
auto useConfig(C c) {
  return boost::di::bind<std::decay_t<C>>().template to(
      std::move(c))[boost::di::override];
}

template <typename T, typename Fun>
auto bind(const Fun &fun) {
  return di::bind<T>.to([fun](auto const &injector) {
    static boost::optional<sptr<T>> initialized = boost::none;
    if (not initialized) {
      initialized.emplace(std::move(fun(injector)));
    }
    return initialized.value();
  });
}

namespace {
  std::string embedded_config(R"(
# ----------------
sinks:
  - name: console
    type: console
    thread: none
    color: false
    latency: 0
groups:
  - name: main
    sink: console
    level: trace
    is_fallback: true
    children:
      - name: libp2p
        level: off
      - name: network
      - name: peering-utility
# ----------------
)");
}

const char *bootnodesArr[] = {
    "/dns/p2p.0.polkadot.network/tcp/30333/p2p/"
    "12D3KooWHsvEicXjWWraktbZ4MQBizuyADQtuEGr3NbDvtm5rFA5",
    "/dns/p2p.1.polkadot.network/tcp/30333/p2p/"
    "12D3KooWQz2q2UWVCiy9cFX1hHYEmhSKQB2hjEZCccScHLGUPjcc",
    "/dns/p2p.2.polkadot.network/tcp/30333/p2p/"
    "12D3KooWNHxjYbDLLbDNZ2tq1kXgif5MSiLTUWJKcDdedKu4KaG8",
    "/dns/p2p.3.polkadot.network/tcp/30333/p2p/"
    "12D3KooWGJQysxrQcSvUWWNw88RkqYvJhH3ZcDpWJ8zrXKhLP5Vr",
    "/dns/p2p.4.polkadot.network/tcp/30333/p2p/"
    "12D3KooWKer8bYqpYjwurVABu13mkELpX2X7mSpEicpjShLeg7D6",
    "/dns/p2p.5.polkadot.network/tcp/30333/p2p/"
    "12D3KooWSRjL9LcEQd5u2fQTbyLxTEHq1tUFgQ6amXSp8Eu7TfKP",
    "/dns/cc1-0.parity.tech/tcp/30333/p2p/"
    "12D3KooWSz8r2WyCdsfWHgPyvD8GKQdJ1UAiRmrcrs8sQB3fe2KU",
    "/dns/cc1-1.parity.tech/tcp/30333/p2p/"
    "12D3KooWFN2mhgpkJsDBuNuE5427AcDrsib8EoqGMZmkxWwx3Md4"};

class Configurator : public soralog::ConfiguratorFromYAML {
 public:
  Configurator() : ConfiguratorFromYAML(embedded_config) {}
};

class PeerManager {
 public:
  virtual ~PeerManager() = default;
};

template <typename Injector>
class PeerManagerImpl
    : public PeerManager,
      public std::enable_shared_from_this<PeerManagerImpl<Injector>> {
 public:
  PeerManagerImpl(const Injector &injector)
      : app_state_manager_(injector.template create<
                           std::shared_ptr<application::AppStateManager>>()),
        host_(injector.template create<sptr<libp2p::Host>>()),
        identify_(injector.template create<sptr<libp2p::protocol::Identify>>()),
        kademlia_(
            injector
                .template create<sptr<libp2p::protocol::kademlia::Kademlia>>()),
        ping_protocol_(
            injector.template create<sptr<libp2p::protocol::Ping>>()),
        grandpa_protocol_(
            injector.template create<sptr<network::GrandpaProtocol>>()),
        log_(log::createLogger("PeerManager", "network")) {
    app_state_manager_->takeControl(*this);
  }

  ~PeerManagerImpl() override {
    ;
  }

  bool prepare() {
    return true;
  }

  bool start() {
    kademlia_->addPeer(host_->getPeerInfo(), true);

    for (const auto &item : bootnodesArr) {
      auto addr = libp2p::multi::Multiaddress::create(item).value();
      kademlia_->addPeer(
          libp2p::peer::PeerInfo{
              .id = libp2p::peer::PeerId::fromBase58(addr.getPeerId().value())
                        .value(),
              .addresses = {addr}},
          true);
    }

    add_peer_handle_ =
        host_->getBus()
            .getChannel<libp2p::event::protocol::kademlia::PeerAddedChannel>()
            .subscribe([wp = this->weak_from_this()](
                           const libp2p::peer::PeerId &peer_id) {
              if (auto self = wp.lock()) {
                self->log_->trace("{}", peer_id.toBase58());
              }
            });

    host_->setProtocolHandler(libp2p::protocol::detail::kPingProto,
                              [wp = this->weak_from_this()](auto &&stream) {
                                if (auto self = wp.lock()) {
                                  if (auto peer_id = stream->remotePeerId()) {
                                    self->log_->info(
                                        "Handled ping protocol stream from: {}",
                                        peer_id.value().toBase58());
                                    self->ping_protocol_->handle(
                                        std::forward<decltype(stream)>(stream));
                                  }
                                }
                              });

    identify_->onIdentifyReceived(
        [wp = this->weak_from_this()](const libp2p::peer::PeerId &peer_id) {
          if (auto self = wp.lock()) {
            self->log_->trace("Identify received from peer_id={}",
                              peer_id.toBase58());
            auto addresses_res =
                self->host_->getPeerRepository().getAddressRepository().getAddresses(
                    peer_id);
            if (addresses_res.has_value()) {
              auto &addresses = addresses_res.value();
              libp2p::peer::PeerInfo peer_info{.id = peer_id,
                                 .addresses = std::move(addresses)};
              self->kademlia_->addPeer(peer_info, false);
            }
          }
        });

    host_->start();
    identify_->start();
    kademlia_->start();
    grandpa_protocol_->start();
    return true;
  }

  void stop() {
    add_peer_handle_.unsubscribe();
  }

 private:
  sptr<application::AppStateManager> app_state_manager_;
  sptr<libp2p::Host> host_;
  sptr<libp2p::protocol::Identify> identify_;
  sptr<libp2p::protocol::kademlia::Kademlia> kademlia_;
  sptr<libp2p::protocol::Ping> ping_protocol_;
  sptr<network::GrandpaProtocol> grandpa_protocol_;
  log::Logger log_;
  libp2p::event::Handle add_peer_handle_;
};

int main(int argc, char *argv[]) {
  backward::SignalHandling sh;

  using namespace std::chrono_literals;

  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<Configurator>());
  std::ignore = logging_system->configure();
  log::setLoggingSystem(logging_system);

  auto log = log::createLogger("main", "peering-utility");

  libp2p::protocol::PingConfig ping_config{};

  auto injector = di::make_injector(
      useConfig(ping_config),
      di::bind<PeerManager>.to([](auto const &injector) {
        return std::make_shared<PeerManagerImpl<decltype(injector)>>(injector);
      }),
      di::bind<application::AppStateManager>.template to<application::AppStateManagerImpl>(),
      libp2p::injector::makeHostInjector(
          libp2p::injector::useSecurityAdaptors<
              libp2p::security::Noise>()[di::override]),
      libp2p::injector::makeKademliaInjector(),
      bind<libp2p::protocol::kademlia::Config>([](auto const &) {
        return std::make_shared<libp2p::protocol::kademlia::Config>(
            libp2p::protocol::kademlia::Config{
                .protocolId = "/dot/kad",
                .maxBucketSize = 1000,
                .randomWalk = {.interval = std::chrono::minutes(1)}});
      })[di::override],
      bind<libp2p::crypto::KeyPair>([](auto const &injector) {
        auto &crypto_provider =
            injector.template create<const crypto::Ed25519Provider &>();

        auto generated_keypair = crypto_provider.generateKeypair();

        auto &&pub = generated_keypair.public_key;
        auto &&priv = generated_keypair.secret_key;

        auto key_pair =
            std::make_shared<libp2p::crypto::KeyPair>(libp2p::crypto::KeyPair{
                .publicKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                               .data = {pub.begin(), pub.end()}}},
                .privateKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                                .data = {priv.begin(), priv.end()}}}});

        return key_pair;
      })[di::override],
      di::bind<crypto::Ed25519Provider>.template to<crypto::Ed25519ProviderImpl>(),
      di::bind<::boost::asio::io_context>.in(
          di::extension::shared)[di::override],
      di::bind<libp2p::crypto::random::RandomGenerator>.template to<libp2p::crypto::random::BoostRandomGenerator>()
          [di::override]);

  auto peer_manager = injector.template create<sptr<PeerManager>>();

  auto app_state_manager =
      injector.template create<sptr<application::AppStateManager>>();

  auto io_context = injector.template create<sptr<boost::asio::io_context>>();

  app_state_manager->atLaunch([ctx{io_context}] {
    std::thread asio_runner([ctx{ctx}] { ctx->run(); });
    asio_runner.detach();
    return true;
  });

  app_state_manager->atShutdown([ctx{io_context}] { ctx->stop(); });

  app_state_manager->run();
}
