/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_SERVICE_IMPL_HPP
#define KAGOME_TELEMETRY_SERVICE_IMPL_HPP

#include "telemetry/service.hpp"

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson {
  typedef ::std::size_t SizeType;
}
#include <rapidjson/document.h>

#include <boost/asio/io_context.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/host/host.hpp>
#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "common/spin_lock.hpp"
#include "log/logger.hpp"
#include "network/peer_manager.hpp"
#include "storage/buffer_map_types.hpp"
#include "telemetry/connection.hpp"
#include "telemetry/impl/message_pool.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::telemetry {

  static constexpr auto kImplementationName = "Kagome Node";
  static constexpr auto kImplementationVersion = "0.7.7";

  static constexpr auto kTelemetryReportingInterval = std::chrono::seconds(1);
  static constexpr auto kTelemetrySystemReportInterval =
      std::chrono::seconds(5);
  static constexpr auto kTelemetryMessageMaxLengthBytes = 2 * 1024;
  static constexpr auto kTelemetryMessagePoolSize = 1000;

  class TelemetryServiceImpl
      : public TelemetryService,
        public std::enable_shared_from_this<TelemetryService> {
   public:
    TelemetryServiceImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        const application::AppConfiguration &app_configuration,
        const application::ChainSpec &chain_spec,
        const libp2p::Host &host,
        std::shared_ptr<const transaction_pool::TransactionPool> tx_pool,
        std::shared_ptr<const storage::BufferStorage> buffer_storage,
        std::shared_ptr<const network::PeerManager> peer_manager);
    TelemetryServiceImpl(const TelemetryServiceImpl &) = delete;
    TelemetryServiceImpl(TelemetryServiceImpl &&) = delete;
    TelemetryServiceImpl &operator=(const TelemetryServiceImpl &) = delete;
    TelemetryServiceImpl &operator=(TelemetryServiceImpl &&) = delete;

    void notifyBlockImported(const primitives::BlockInfo &info,
                             BlockOrigin origin) override;

    void notifyBlockFinalized(const primitives::BlockInfo &info) override;

    void setGenesisBlockHash(const primitives::BlockHash &hash) override;

   private:
    // handlers for AppStateManager
    bool prepare();
    bool start();
    void stop();

    /// produces the greeting message for the (re-)established connections
    std::string connectedMessage();

    /// produces and sends notifications about best and finalized block
    void frequentNotificationsRoutine();

    /// produces and sends system health notifications
    void delayedNotificationsRoutine();

    /**
     * Constructs the main and immutable part of JSON to be serialized later as
     * greeting message on new telemetry connections.
     */
    void prepareGreetingMessage();

    /**
     * Produces "block.imported" or "notify.finalized" JSON telemetry messages
     * @param info - block info to notify about
     * @param origin - if set, then "block.imported" event produced, otherwise
     * "notify.finalized"
     * @return string with event JSON
     */
    std::string blockNotification(const primitives::BlockInfo &info,
                                  std::optional<BlockOrigin> origin);

    /// compose system health notification of the first format
    std::string systemIntervalMessage1();

    /// compose system health notification of the second format
    std::string systemIntervalMessage2();

    /// @return RFC3339 formatted current timestamp string
    std::string currentTimestamp() const;

    /*****************************************************************
     *                                                               *
     *  Class Fields                                                 *
     *                                                               *
     *****************************************************************/

    // constructor arguments
    std::shared_ptr<application::AppStateManager> app_state_manager_;
    const application::AppConfiguration &app_configuration_;
    const application::ChainSpec &chain_spec_;
    const libp2p::Host &host_;
    std::shared_ptr<const transaction_pool::TransactionPool> tx_pool_;
    std::shared_ptr<const storage::BufferStorage> buffer_storage_;
    std::shared_ptr<const network::PeerManager> peer_manager_;

    // connections thread fields
    volatile bool shutdown_requested_ = false;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    using WorkGuardT = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;
    std::shared_ptr<WorkGuardT> work_guard_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<std::thread> worker_thread_;
    std::vector<std::shared_ptr<TelemetryConnection>> connections_;
    libp2p::basic::Scheduler::Handle frequent_timer_;
    libp2p::basic::Scheduler::Handle delayed_timer_;

    // data cache before serialization and sending over
    common::spin_lock cache_mutex_;
    struct {
      bool is_set = false;
      primitives::BlockInfo block{0, {}};
      BlockOrigin origin;
    } last_imported_;
    struct {
      primitives::BlockNumber reported = 0;
      primitives::BlockInfo block{0, {}};
    } last_finalized_;

    // auxiliary fields
    log::Logger log_;
    rapidjson::Document greeting_json_;
    std::string genesis_hash_;
    MessagePool message_pool_;
  };

}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_SERVICE_IMPL_HPP
