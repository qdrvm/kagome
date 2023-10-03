/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry/service.hpp"

#include <boost/assert.hpp>

namespace kagome::telemetry {

  namespace {

    class TelemetryInstanceProxy : public TelemetryService {
     public:
      void setGenesisBlockHash(const primitives::BlockHash &hash) override {
        if (service_) {
          service_->setGenesisBlockHash(hash);
        }
        genesis_hash_ = hash;
      }
      void notifyBlockImported(const primitives::BlockInfo &info,
                               BlockOrigin origin) override {
        if (service_) {
          service_->notifyBlockImported(info, origin);
        }
      }
      void notifyBlockFinalized(const primitives::BlockInfo &info) override {
        if (service_) {
          service_->notifyBlockFinalized(info);
        }
      }
      void pushBlockStats() override {
        if (service_) {
          service_->pushBlockStats();
        }
      }
      void notifyWasSynchronized() override {
        if (service_) {
          service_->notifyWasSynchronized();
        }
        was_synchronized_ = true;
      }
      bool isEnabled() const override {
        return false;
      }

      void setActualImplementation(Telemetry service) {
        service_ = std::move(service);
        if (not service_) {
          return;
        }
        if (was_synchronized_) {
          service_->notifyWasSynchronized();
        }
        service_->setGenesisBlockHash(genesis_hash_);
      }

     private:
      bool was_synchronized_ = false;
      primitives::BlockHash genesis_hash_{};
      Telemetry service_;
    };

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::shared_ptr<TelemetryInstanceProxy> telemetry_service_;

  }  // namespace

  void setTelemetryService(std::shared_ptr<TelemetryService> service) {
    if (not telemetry_service_) {
      telemetry_service_ = std::make_shared<TelemetryInstanceProxy>();
    }
    telemetry_service_->setActualImplementation(std::move(service));
  }

  std::shared_ptr<TelemetryService> createTelemetryService() {
    if (not telemetry_service_) {
      telemetry_service_ = std::make_shared<TelemetryInstanceProxy>();
    }
    return telemetry_service_;
  }

}  // namespace kagome::telemetry
