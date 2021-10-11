/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_worker_api.hpp"

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
      const primitives::BlockHash &block, primitives::BlockNumber number) {
    log::Logger log = log::createLogger("OCW#" + std::to_string(number),
                                        "offchain_worker_api");

    auto worker =
        [executor = executor_, block, number, log = std::move(log)]() {
          auto res = executor->callAt<void>(
              block, "OffchainWorkerApi_offchain_worker", number);
          if (res.has_failure()) {
            log->error(
                "Can't execute offchain worker for block #{} hash={}: {}",
                number,
                block.toHex(),
                res.error().message());
            return;
          }
          SL_DEBUG(log,
                   "Offchain worker for block #{} hash={} executed successful",
                   number,
                   block.toHex());
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
