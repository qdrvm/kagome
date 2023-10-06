/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/requests/health.hpp"

#include "api/jrpc/value_converter.hpp"
#include "api/service/system/system_api.hpp"

namespace kagome::api::system::request {

  Health::Health(std::shared_ptr<SystemApi> api) : api_(std::move(api)) {
    BOOST_ASSERT(api_ != nullptr);
  }

  outcome::result<void> Health::init(
      const jsonrpc::Request::Parameters &params) {
    if (!params.empty()) {
      throw jsonrpc::InvalidParametersFault("Method should not have params");
    }
    return outcome::success();
  }

  outcome::result<jsonrpc::Value::Struct> Health::execute() {
    jsonrpc::Value::Struct data;

    // isSyncing - Whether the node is syncing.
    data["isSyncing"] = makeValue(api_->getTimeline()->getCurrentState()
                                  != consensus::SyncState::SYNCHRONIZED);

    // peers - Number of connected peers
    data["peers"] = makeValue(
        static_cast<uint64_t>(api_->getPeerManager()->activePeersNumber()));

    // shouldHavePeers - Should this node have any peers.
    // Might be false for local chains or when running without discovery.
    data["shouldHavePeers"] =
        makeValue(api_->getConfig()->chainType() != "Development");

    return data;
  }

}  // namespace kagome::api::system::request
