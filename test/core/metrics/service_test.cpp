#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/di.hpp>
#include <boost/outcome.hpp>
#include <memory>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <thread>
#include "application/impl/app_state_manager_impl.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"
#include "metrics/impl/exposer_impl.hpp"
#include "metrics/impl/prometheus/handler_impl.hpp"
#include "metrics/metrics.hpp"

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
    level: debug
    is_fallback: true
    children:
      - name: kagome
        children:
          - name: metrics
# ----------------
)");
}

class Configurator : public soralog::ConfiguratorFromYAML {
 public:
  Configurator() : ConfiguratorFromYAML(embedded_config) {}
};

class HttpClient {
  boost::asio::io_context ioc_;
  boost::beast::tcp_stream stream_;
  boost::asio::ip::tcp::endpoint endpoint_;

 public:
  HttpClient()
      : ioc_{},
        stream_{ioc_},
        endpoint_{boost::asio::ip::address::from_string("0.0.0.0"), 9955} {}

  ~HttpClient() {
    boost::system::error_code ec;
    stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    boost::ignore_unused(ec);
  }

  boost::system::error_code connect() {
    boost::system::error_code ec;
    stream_.connect(endpoint_, ec);
    return ec;
  }

  boost::outcome_v2::result<std::string, boost::system::error_code> query() {
    boost::system::error_code ec;
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::get, "/", 11};
    boost::beast::http::write(stream_, req, ec);
    if (ec) {
      return ec;
    }
    boost::beast::flat_buffer buffer{};
    boost::beast::http::response<boost::beast::http::string_body> res{};
    boost::beast::http::read(stream_, buffer, res, ec);
    if (ec) {
      return ec;
    }
    return res.body();
  }
};

/**
 * @given an empty metrics metering service with minimal app maintenance
 * @when adding simple metrics
 * @then get expected response from service endpoint
 */
TEST(ServiceTest, CreateMetricsExposer) {
  using namespace kagome;
  namespace di = boost::di;

  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<Configurator>());
  logging_system->configure();
  log::setLoggingSystem(logging_system);

  auto injector = di::make_injector(
      di::bind<metrics::Handler>.template to<metrics::PrometheusHandler>(),
      di::bind<metrics::Exposer>.template to<metrics::ExposerImpl>(),
      di::bind<application::AppStateManager>.template to<application::AppStateManagerImpl>(),
      di::bind<metrics::Exposer::Configuration>.to([](const auto &) {
        return metrics::Exposer::Configuration{
            {boost::asio::ip::address::from_string("0.0.0.0"), 9955}};
      }),
      di::bind<metrics::Session::Configuration>.to([](const auto &injector) {
        return metrics::Session::Configuration{};
      }));

  auto app_state_manager =
      injector.template create<std::shared_ptr<application::AppStateManager>>();
  auto registry = kagome::metrics::createRegistry();
  auto handler = injector.create<std::shared_ptr<kagome::metrics::Handler>>();
  registry->setHandler(*handler.get());
  auto exposer = injector.create<std::shared_ptr<kagome::metrics::Exposer>>();
  exposer->setHandler(handler);

  registry->registerCounterFamily("counter", "It's simple counter!");
  auto counter = registry->registerCounterMetric("counter");
  counter->inc();

  exposer->prepare();
  exposer->start();

  HttpClient client;
  auto ec = client.connect();
  ASSERT_FALSE(ec);
  auto res = client.query();
  ASSERT_FALSE(res.has_error());
  std::string answer(R"(# HELP counter It's simple counter!
# TYPE counter counter
counter 1
)");
  ASSERT_STREQ(res.value().c_str(), answer.c_str());

  exposer->stop();
}
