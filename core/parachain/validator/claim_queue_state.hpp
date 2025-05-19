#pragma once

#include <optional>
#include <unordered_map>

#include "parachain/types.hpp"
#include "primitives/common.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {

  /**
   * @brief Tracks claim queue state for collation fairness
   *
   * This class helps ensure that parachains receive fair amounts of core time
   * based on their positions in the claim queue
   */
  class ClaimQueueState {
   public:
    using RelayParentHash = primitives::BlockHash;

    struct ParaClaimState {
      // Number of claims in the claim queue
      size_t num_claims = 0;
      // Number of seconded and active fetch attempts
      size_t num_active = 0;
    };

    /**
     * @brief Update claim queue state with new information
     */
    void updateClaimQueue(const RelayParentHash &relay_parent,
                          const runtime::ClaimQueueSnapshot &claim_queue);

    /**
     * @brief Check if this parachain should be allowed to claim at this relay
     * parent
     */
    bool canClaimAt(const RelayParentHash &relay_parent,
                    const ParachainId &para_id);

    /**
     * @brief Register a fetch attempt for this parachain
     */
    void registerFetchAttempt(const RelayParentHash &relay_parent,
                              const ParachainId &para_id);

    /**
     * @brief Complete a fetch attempt for this parachain
     */
    void completeFetchAttempt(const RelayParentHash &relay_parent,
                              const ParachainId &para_id);

    // State per relay parent and parachain - made public for direct access by
    // ValidatorSide
    std::unordered_map<RelayParentHash,
                       std::unordered_map<ParachainId, ParaClaimState>>
        state_by_relay_parent_and_para_;
  };

}  // namespace kagome::parachain
