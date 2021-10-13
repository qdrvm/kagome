/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_worker_api.hpp"

#include <soralog/util.hpp>
#include <thread>

#include "log/logger.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {

  OffchainWorkerApiImpl::OffchainWorkerApiImpl(
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<void> OffchainWorkerApiImpl::offchain_worker(
      const primitives::BlockHash &block,
      const primitives::BlockHeader &header) {
    auto worker = [executor = executor_, hash = block, header]() {
      auto number = header.number;

      soralog::util::setThreadName("ocw." + std::to_string(number));
      log::Logger log = log::createLogger(
          "OffchainWorker#" + std::to_string(number), "offchain_worker_api");

      SL_TRACE(log,
               "Offchain worker is started for block #{} hash={}",
               number,
               hash.toHex());


      auto res = executor->callAt<void>(
          header.parent_hash, "OffchainWorkerApi_offchain_worker", header);
      if (res.has_failure()) {
        log->error("Can't execute offchain worker for block #{} hash={}: {}",
                   number,
                   hash.toHex(),
                   res.error().message());
        return;
      }

      SL_DEBUG(log,
               "Offchain worker is successfully executed for block #{} hash={}",
               number,
               hash.toHex());
    };

    try {
      std::thread(std::move(worker)).detach();
    } catch (const std::system_error &exception) {
      return outcome::failure(exception.code());
    } catch (...) {
      return outcome::failure(boost::system::error_code{});
    }

    return outcome::success();
  }

}  // namespace kagome::runtime
