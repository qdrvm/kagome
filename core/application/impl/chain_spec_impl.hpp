/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_SPEC_IMPL_HPP
#define KAGOME_CHAIN_SPEC_IMPL_HPP

#include "application/chain_spec.hpp"

#include <boost/property_tree/ptree.hpp>

#include "log/logger.hpp"
#include "primitives/code_substitutes.hpp"

namespace kagome::application {

  class ChainSpecImpl : public ChainSpec {
   public:
    enum class Error {
      MISSING_ENTRY = 1,
      MISSING_PEER_ID,
      PARSER_ERROR,
      NOT_IMPLEMENTED
    };

    static outcome::result<std::shared_ptr<ChainSpecImpl>> loadFrom(
        const std::string &config_path);

    ~ChainSpecImpl() override = default;

    const std::string &name() const override {
      return name_;
    }

    const std::string &id() const override {
      return id_;
    }

    const std::string &chainType() const override {
      return chain_type_;
    }

    const std::vector<libp2p::multi::Multiaddress> &bootNodes() const override {
      return boot_nodes_;
    }

    const std::vector<std::pair<std::string, size_t>> &telemetryEndpoints()
        const override {
      return telemetry_endpoints_;
    }

    const std::string &protocolId() const override {
      return protocol_id_;
    }

    const std::map<std::string, std::string> &properties() const override {
      return properties_;
    }

    std::optional<std::reference_wrapper<const std::string>> getProperty(
        const std::string &property) const override {
      auto it = properties_.find(property);
      if (it != properties_.end()) {
        return {{it->second}};
      }
      return std::nullopt;
    }

    const std::set<primitives::BlockHash> &forkBlocks() const override {
      return fork_blocks_;
    }

    const std::set<primitives::BlockHash> &badBlocks() const override {
      return bad_blocks_;
    }

    std::optional<std::string> consensusEngine() const override {
      return consensus_engine_;
    }

    std::shared_ptr<const primitives::CodeSubstituteHashes> codeSubstitutes()
        const override {
      return known_code_substitutes_;
    }

    GenesisRawData getGenesis() const override {
      return genesis_;
    }

    outcome::result<common::Buffer> fetchCodeSubstituteByHash(
        const common::Hash256 &hash) const override;

   private:
    outcome::result<void> loadFromJson(const std::string &file_path);
    outcome::result<void> loadFields(const boost::property_tree::ptree &tree);
    outcome::result<void> loadGenesis(const boost::property_tree::ptree &tree);
    outcome::result<void> loadBootNodes(
        const boost::property_tree::ptree &tree);

    template <typename T>
    outcome::result<std::decay_t<T>> ensure(std::string_view entry_name,
                                            boost::optional<T> opt_entry) {
      if (not opt_entry) {
        log_->error("Required '{}' entry not found in the chain spec",
                    entry_name);
        return Error::MISSING_ENTRY;
      }
      return opt_entry.value();
    }

    ChainSpecImpl() = default;

    std::string name_;
    std::string id_;
    std::string chain_type_;
    std::string config_path_;
    std::vector<libp2p::multi::Multiaddress> boot_nodes_;
    std::vector<std::pair<std::string, size_t>> telemetry_endpoints_;
    std::string protocol_id_{"sup"};
    std::map<std::string, std::string> properties_;
    std::set<primitives::BlockHash> fork_blocks_;
    std::set<primitives::BlockHash> bad_blocks_;
    std::optional<std::string> consensus_engine_;
    std::shared_ptr<primitives::CodeSubstituteHashes> known_code_substitutes_;
    GenesisRawData genesis_;
    log::Logger log_ = log::createLogger("chain_spec", "kagome");
  };

}  // namespace kagome::application

OUTCOME_HPP_DECLARE_ERROR(kagome::application, ChainSpecImpl::Error)

#endif  // KAGOME_CHAIN_SPEC_IMPL_HPP
