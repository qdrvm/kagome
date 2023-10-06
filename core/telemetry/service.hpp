/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_SERVICE_HPP
#define KAGOME_TELEMETRY_SERVICE_HPP

#include <string>

#include "primitives/common.hpp"

namespace kagome::telemetry {

  /**
   * Possible block origins enumeration
   *
   * https://github.com/paritytech/substrate/blob/42b2d623d058197aebc3c737fb44fbbf278a85b4/primitives/consensus/common/src/lib.rs#L64
   */
  enum class BlockOrigin {
    /// Genesis block built into the client.
    kGenesis,

    /// Block is part of the initial sync with the network.
    kNetworkInitialSync,

    /// Block was broadcasted on the network.
    kNetworkBroadcast,

    /// Block that was received from the network and validated in the consensus
    /// process.
    kConsensusBroadcast,

    /// Block that was collated by this node.
    kOwn,

    /// Block was imported from a file.
    kFile,
  };

  /**
   * Telemetry service interface
   */
  class TelemetryService {
   public:
    virtual ~TelemetryService() = default;

    /**
     * Used to initially inform the service about the genesis hash.
     * @param hash genesis hash for the network
     *
     * Allows to avoid circular references in classes dependency tree
     */
    virtual void setGenesisBlockHash(const primitives::BlockHash &hash) = 0;

    /**
     * Let the telemetry service know that the node has been in a synchronized
     * state at least once.
     *
     * After this call all kNetworkInitialSync events will be treated as
     * kNetworkBroadcast
     */
    virtual void notifyWasSynchronized() = 0;

    /**
     * Inform about last known block
     * @param info - block info
     * @param origin - source of the block
     */
    virtual void notifyBlockImported(const primitives::BlockInfo &info,
                                     BlockOrigin origin) = 0;

    /**
     * Inform about the last finalized block
     * @param info - block info
     */
    virtual void notifyBlockFinalized(const primitives::BlockInfo &info) = 0;

    /**
     * Send imported+finalized blocks info immediately and reset periodic timer
     */
    virtual void pushBlockStats() = 0;

    /**
     * Telemetry service status
     * @return true - when application configured to broadcast telemetry
     */
    virtual bool isEnabled() const = 0;
  };

  using Telemetry = std::shared_ptr<TelemetryService>;

  /// Sets an instance of telemetry service for latter usage by reporters
  void setTelemetryService(Telemetry service);

  /// Returns preliminary initialized instance of telemetry service
  Telemetry createTelemetryService();

}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_SERVICE_HPP
