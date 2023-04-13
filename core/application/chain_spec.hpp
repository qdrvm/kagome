/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_SPEC_HPP
#define KAGOME_CHAIN_SPEC_HPP

#include <libp2p/peer/peer_info.hpp>
#include <set>

#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/block.hpp"
#include "primitives/code_substitutes.hpp"

namespace kagome::application {

  using GenesisRawData = std::vector<std::pair<common::Buffer, common::Buffer>>;
  using ChildrenDefaultRawData = std::map<common::Buffer, GenesisRawData>;

  /**
   * Stores configuration of a kagome node and provides convenience
   * methods for accessing config parameters
   */
  class ChainSpec {
   public:
    virtual ~ChainSpec() = default;

    virtual const std::string &name() const = 0;

    virtual const std::string &id() const = 0;

    virtual const std::string &chainType() const = 0;

    /// Return ids of peer nodes of the current node
    virtual const std::vector<libp2p::multi::Multiaddress> &bootNodes()
        const = 0;

    virtual const std::vector<std::pair<std::string, size_t>>
        &telemetryEndpoints() const = 0;

    virtual const std::string &protocolId() const = 0;

    virtual const std::map<std::string, std::string> &properties() const = 0;

    virtual std::optional<std::reference_wrapper<const std::string>>
    getProperty(const std::string &property) const = 0;

    virtual const std::set<primitives::BlockHash> &forkBlocks() const = 0;

    virtual const std::set<primitives::BlockHash> &badBlocks() const = 0;

    virtual std::optional<std::string> consensusEngine() const = 0;

    /// Fetches code_substitute from json config on demand, by its BlockInfo.
    /// BlockInfo is being compared with BlockIds that were loaded on initial
    /// configuration and stored in set.
    virtual outcome::result<common::Buffer> fetchCodeSubstituteByBlockInfo(
        const primitives::BlockInfo &block_info) const = 0;

    /**
     * @return runtime code substitution map
     */
    virtual std::shared_ptr<const primitives::CodeSubstituteBlockIds>
    codeSubstitutes() const = 0;

    /**
     * @return top section
     */
    virtual const GenesisRawData &getGenesisTopSection() const = 0;

    /**
     * @return childrenDefault section
     */
    virtual const ChildrenDefaultRawData &getGenesisChildrenDefaultSection()
        const = 0;

    bool idStartsWith(std::string_view prefix) const {
      return id().rfind(prefix, 0) == 0;
    }

    bool isKusama() const {
      return idStartsWith("kusama") || idStartsWith("ksm");
    }

    bool isRococo() const {
      return idStartsWith("rococo") || idStartsWith("rco");
    }

    bool isWococo() const {
      return idStartsWith("wococo") || idStartsWith("wco");
    }

    bool isVersi() const {
      return idStartsWith("versi") || idStartsWith("vrs");
    }
  };

}  // namespace kagome::application

#endif  // KAGOME_CHAIN_SPEC_HPP
