/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_FRAGMENT_TREE
#define KAGOME_PARACHAIN_FRAGMENT_TREE

#include <boost/variant.hpp>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "outcome/outcome.hpp"

#include "log/logger.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/collations.hpp"
#include "primitives/common.hpp"
#include "primitives/math.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/map.hpp"

namespace kagome::parachain::fragment {

  template <typename... Args>
  using HashMap = std::unordered_map<Args...>;
  template <typename... Args>
  using HashSet = std::unordered_set<Args...>;
  template <typename... Args>
  using Vec = std::vector<Args...>;
  using BitVec = scale::BitVec;
  using ParaId = ParachainId;
  template <typename... Args>
  using Option = std::optional<Args...>;
  template <typename... Args>
  using Map = std::map<Args...>;
  using FragmentTreeMembership = Vec<std::pair<Hash, Vec<size_t>>>;

  struct ProspectiveCandidate {
    /// The commitments to the output of the execution.
    network::CandidateCommitments commitments;
    /// The collator that created the candidate.
    CollatorId collator;
    /// The signature of the collator on the payload.
    runtime::CollatorSignature collator_signature;
    /// The persisted validation data used to create the candidate.
    runtime::PersistedValidationData persisted_validation_data;
    /// The hash of the PoV.
    Hash pov_hash;
    /// The validation code hash used by the candidate.
    ValidationCodeHash validation_code_hash;
  };

  /// The state of a candidate.
  ///
  /// Candidates aren't even considered until they've at least been seconded.
  enum CandidateState {
    /// The candidate has been introduced in a spam-protected way but
    /// is not necessarily backed.
    Introduced,
    /// The candidate has been seconded.
    Seconded,
    /// The candidate has been completely backed by the group.
    Backed,
  };

  struct CandidateEntry {
    CandidateHash candidate_hash;
    RelayHash relay_parent;
    ProspectiveCandidate candidate;
    CandidateState state;
  };

  struct CandidateStorage {
    enum class Error {
      CANDIDATE_ALREADY_KNOWN,
      PERSISTED_VALIDATION_DATA_MISMATCH,
    };

    // Index from head data hash to candidate hashes with that head data as a
    // parent.
    HashMap<Hash, HashSet<CandidateHash>> by_parent_head;

    // Index from head data hash to candidate hashes outputting that head data.
    HashMap<Hash, HashSet<CandidateHash>> by_output_head;

    // Index from candidate hash to fragment node.
    HashMap<CandidateHash, CandidateEntry> by_candidate_hash;

    outcome::result<void> addCandidate(
        const CandidateHash &candidate_hash,
        const network::CommittedCandidateReceipt &candidate,
        const crypto::Hashed<runtime::PersistedValidationData, 32>
            &persisted_validation_data,
        const std::shared_ptr<crypto::Hasher> &hasher);

    Option<std::reference_wrapper<const CandidateEntry>> get(
        const CandidateHash &candidate_hash) const {
      if (auto it = by_candidate_hash.find(candidate_hash);
          it != by_candidate_hash.end()) {
        return {{it->second}};
      }
      return std::nullopt;
    }

    bool contains(const CandidateHash &candidate_hash) const {
      return by_candidate_hash.contains(candidate_hash);
    }

    template <typename F>
    void iterParaChildren(const Hash &parent_head_hash, F &&func) const {
      if (auto it = by_parent_head.find(parent_head_hash);
          it != by_parent_head.end()) {
        for (const auto &h : it->second) {
          if (auto c_it = by_candidate_hash.find(h);
              c_it != by_candidate_hash.end()) {
            std::forward<F>(func)(c_it->second);
          }
        }
      }
    }

    Option<std::reference_wrapper<const HeadData>> headDataByHash(
        const Hash &hash) const {
      auto search = [&](const auto &container)
          -> Option<std::reference_wrapper<const CandidateEntry>> {
        if (auto it = container.find(hash); it != container.end()) {
          if (!it->second.empty()) {
            const CandidateHash &a_candidate = *it->second.begin();
            return get(a_candidate);
          }
        }
      };

      if (auto e = search(by_output_head)) {
        return {{e->get().candidate.commitments.para_head}};
      }
      if (auto e = search(by_parent_head)) {
        return {{e->get().candidate.persisted_validation_data.parent_head}};
      }
      return std::nullopt;
    }

    void removeCandidate(const CandidateHash &candidate_hash,
                         const std::shared_ptr<crypto::Hasher> &hasher) {
      if (auto it = by_candidate_hash.find(candidate_hash);
          it != by_candidate_hash.end()) {
        const auto parent_head_hash = hasher.blake2b_256(
            it->second.candidate.persisted_validation_data.parent_head);
        if (auto it_bph = by_parent_head.find(parent_head_hash);
            it_bph != by_parent_head.end()) {
          it_bph->second.erase(candidate_hash);
          if (it_bph->second.empty()) {
            by_parent_head.erase(it_bph);
          }
        }
        by_candidate_hash.erase(it);
      }
    }

    void markSeconded(const CandidateHash &candidate_hash) {
      if (auto it = by_candidate_hash.find(candidate_hash);
          it != by_candidate_hash.end()) {
        if (it->second.state != CandidateState::Backed) {
          it->second.state = CandidateState::Seconded;
        }
      }
    }

    void markBacked(const CandidateHash &candidate_hash) {
      if (auto it = by_candidate_hash.find(candidate_hash);
          it != by_candidate_hash.end()) {
        it->second.state = CandidateState::Backed;
      }
    }

    bool isBacked(const CandidateHash &candidate_hash) const {
      return by_candidate_hash.count(candidate_hash) > 0
          && by_candidate_hash.at(candidate_hash).state
                 == CandidateState::Backed;
    }
  };

  using NodePointerRoot = network::Empty;
  using NodePointerStorage = size_t;
  using NodePointer = boost::variant<NodePointerRoot, NodePointerStorage>;

  struct RelayChainBlockInfo {
    /// The hash of the relay-chain block.
    Hash hash;
    /// The number of the relay-chain block.
    BlockNumber number;
    /// The storage-root of the relay-chain block.
    Hash storage_root;
  };

  /// Constraints on inbound HRMP channels.
  struct InboundHrmpLimitations {
    /// An exhaustive set of all valid watermarks, sorted ascending
    Vec<BlockNumber> valid_watermarks;
  };

  /// Constraints on outbound HRMP channels.
  struct OutboundHrmpChannelLimitations {
    /// The maximum bytes that can be written to the channel.
    size_t bytes_remaining;
    /// The maximum messages that can be written to the channel.
    size_t messages_remaining;
  };

  struct HrmpWatermarkUpdateHead {
    BlockNumber v;
  };
  struct HrmpWatermarkUpdateTrunk {
    BlockNumber v;
  };
  using HrmpWatermarkUpdate =
      boost::variant<HrmpWatermarkUpdateHead, HrmpWatermarkUpdateTrunk>;
  inline BlockNumber fromHrmpWatermarkUpdate(const HrmpWatermarkUpdate &value) {
    return visit_in_place(value, [](const auto &val) { return val.v; });
  }

  struct OutboundHrmpChannelModification {
    /// The number of bytes submitted to the channel.
    size_t bytes_submitted;
    /// The number of messages submitted to the channel.
    size_t messages_submitted;
  };

  struct ConstraintModifications {
    /// The required parent head to build upon.
    Option<HeadData> required_parent{};
    /// The new HRMP watermark
    Option<HrmpWatermarkUpdate> hrmp_watermark{};
    /// Outbound HRMP channel modifications.
    HashMap<ParaId, OutboundHrmpChannelModification> outbound_hrmp{};
    /// The amount of UMP messages sent.
    size_t ump_messages_sent{0ull};
    /// The amount of UMP bytes sent.
    size_t ump_bytes_sent{0ull};
    /// The amount of DMP messages processed.
    size_t dmp_messages_processed{0ull};
    /// Whether a pending code upgrade has been applied.
    bool code_upgrade_applied{false};

    void stack(const ConstraintModifications &other) {
      if (other.required_parent) {
        required_parent = other.required_parent;
      }
      if (other.hrmp_watermark) {
        hrmp_watermark = other.hrmp_watermark;
      }

      for (const auto &[id, mods] : other.outbound_hrmp) {
        auto &record = outbound_hrmp[id];
        record.messages_submitted += mods.messages_submitted;
        record.bytes_submitted += mods.bytes_submitted;
      }

      ump_messages_sent += other.ump_messages_sent;
      ump_bytes_sent += other.ump_bytes_sent;
      dmp_messages_processed += other.dmp_messages_processed;
      code_upgrade_applied |= other.code_upgrade_applied;
    }
  };

  struct Constraints {
    SCALE_TIE(14);
    enum class Error {
      DISALLOWED_HRMP_WATERMARK,
      NO_SUCH_HRMP_CHANNEL,
      HRMP_BYTES_OVERFLOW,
      HRMP_MESSAGE_OVERFLOW,
      UMP_MESSAGE_OVERFLOW,
      UMP_BYTES_OVERFLOW,
      DMP_MESSAGE_UNDERFLOW,
      APPLIED_NONEXISTENT_CODE_UPGRADE,
    };

    /// The minimum relay-parent number accepted under these constraints.
    BlockNumber min_relay_parent_number;
    /// The maximum Proof-of-Validity size allowed, in bytes.
    size_t max_pov_size;
    /// The maximum new validation code size allowed, in bytes.
    size_t max_code_size;
    /// The amount of UMP messages remaining.
    size_t ump_remaining;
    /// The amount of UMP bytes remaining.
    size_t ump_remaining_bytes;
    /// The maximum number of UMP messages allowed per candidate.
    size_t max_ump_num_per_candidate;
    /// Remaining DMP queue. Only includes sent-at block numbers.
    Vec<BlockNumber> dmp_remaining_messages;
    /// The limitations of all registered inbound HRMP channels.
    InboundHrmpLimitations hrmp_inbound;
    /// The limitations of all registered outbound HRMP channels.
    HashMap<ParaId, OutboundHrmpChannelLimitations> hrmp_channels_out;
    /// The maximum number of HRMP messages allowed per candidate.
    size_t max_hrmp_num_per_candidate;
    /// The required parent head-data of the parachain.
    HeadData required_parent;
    /// The expected validation-code-hash of this parachain.
    ValidationCodeHash validation_code_hash;
    /// The code upgrade restriction signal as-of this parachain.
    Option<network::UpgradeRestriction> upgrade_restriction;
    /// The future validation code hash, if any, and at what relay-parent
    /// number the upgrade would be minimally applied.
    Option<std::pair<BlockNumber, ValidationCodeHash>> future_validation_code;

    outcome::result<Constraints> applyModifications(
        const ConstraintModifications &modifications) const;
  };

  struct BackingState {
    SCALE_TIE(2);
    /// The state-machine constraints of the parachain.
    Constraints constraints;
    /// The candidates pending availability. These should be ordered, i.e. they should form
    /// a sub-chain, where the first candidate builds on top of the required parent of the
    /// constraints and each subsequent builds on top of the previous head-data.
    std::vector<network::vstaging::CandidatePendingAvailability> pending_availability;
  };

  struct Fragment {
    /// The new relay-parent.
    RelayChainBlockInfo relay_parent;
    /// The constraints this fragment is operating under.
    Constraints operating_constraints;
    /// The core information about the prospective candidate.
    ProspectiveCandidate candidate;
    /// Modifications to the constraints based on the outputs of
    /// the candidate.
    ConstraintModifications modifications;

    const RelayChainBlockInfo &relayParent() const {
      return relay_parent;
    }

    static Option<Fragment> create() {
      /// TODO(iceseer): do
      return std::nullopt;
    }

    const ConstraintModifications &constraintModifications() const {
      return modifications;
    }
  };

  struct FragmentNode {
    // A pointer to the parent node.
    NodePointer parent;
    Fragment fragment;
    CandidateHash candidate_hash;
    size_t depth;
    ConstraintModifications cumulative_modifications;
    Vec<std::pair<NodePointer, CandidateHash>> children;

    const Hash &relayParent() const {
      return fragment.relayParent().hash;
    }

    Option<NodePointer> candidateChild(
        const CandidateHash &candidate_hash) const {
      auto it =
          std::find_if(children.begin(),
                       children.end(),
                       [&](const std::pair<NodePointer, CandidateHash> &p) {
                         return p.second == candidate_hash;
                       });
      if (it != children.end()) {
        return it->first;
      }
      return std::nullopt;
    }
  };

  struct PendingAvailability {
    /// The candidate hash.
    CandidateHash candidate_hash;
    /// The block info of the relay parent.
    RelayChainBlockInfo relay_parent;
  };

  struct Scope {
    ParaId para;
    RelayChainBlockInfo relay_parent;
    Map<BlockNumber, RelayChainBlockInfo> ancestors;
    HashMap<Hash, RelayChainBlockInfo> ancestors_by_hash;
    Vec<PendingAvailability> pending_availability;
    Constraints base_constraints;
    size_t max_depth;

    const RelayChainBlockInfo &earliestRelayParent() const {
      if (!ancestors.empty()) {
        return ancestors.begin()->second;
      }
      return relay_parent;
    }

    Option<std::reference_wrapper<const PendingAvailability>>
    getPendingAvailability(const CandidateHash &candidate_hash) const {
      auto it = std::find_if(pending_availability.begin(),
                             pending_availability.end(),
                             [&](const PendingAvailability &c) {
                               return c.candidate_hash == candidate_hash;
                             });
      if (it != pending_availability.end()) {
        return {{*it}};
      }
      return std::nullopt;
    }

    Option<std::reference_wrapper<const RelayChainBlockInfo>> ancestorByHash(
        const Hash &hash) const {
      if (hash == relay_parent.hash) {
        return {{relay_parent}};
      }
      if (auto it = ancestors_by_hash.find(hash);
          it != ancestors_by_hash.end()) {
        return {{it->second}};
      }
      return std::nullopt;
    }
  };

  /// This is a tree of candidates based on some underlying storage of
  /// candidates and a scope.
  ///
  /// All nodes in the tree must be either pending availability or within the
  /// scope. Within the scope means it's built off of the relay-parent or an
  /// ancestor.
  struct FragmentTree {
    Scope scope;

    // Invariant: a contiguous prefix of the 'nodes' storage will contain
    // the top-level children.
    Vec<FragmentNode> nodes;

    // The candidates stored in this tree, mapped to a bitvec indicating the
    // depths where the candidate is stored.
    HashMap<CandidateHash, BitVec> candidates;

    log::Logger logger = log::createLogger("parachain", "fragment_tree");

    Option<Vec<size_t>> candidate(const CandidateHash &hash) const {
      if (auto it = candidates.find(hash); it != candidates.end()) {
        Vec<size_t> res;
        for (size_t ix = 0; ix < it->second.bits.size(); ++ix) {
          if (it->second.bits[ix]) {
            res.emplace_back(ix);
          }
        }
        return res;
      }
      return std::nullopt;
    }

    static FragmentTree populate(const Scope &scope,
                                 const CandidateStorage &storage) {
      auto logger = log::createLogger("parachain", "fragment_tree");
      SL_TRACE(logger,
               "Instantiating Fragment Tree. (relay parent={}, relay parent "
               "num={}, para id={}, ancestors={})",
               scope.relay_parent.hash,
               scope.relay_parent.number,
               scope.para,
               scope.ancestors.size());

      FragmentTree tree{.scope = scope, .nodes = {}, .candidates = {}};

      tree.populateFromBases(storage, {NodePointerRoot});
      return tree;
    }

    void populateFromBases(const CandidateStorage &storage,
                           const std::vector<NodePointer> &initial_bases) {
      Option<size_t> last_sweep_start{};
      do {
        const auto sweep_start = nodes.size();
        if (last_sweep_start) {
          break;
        }

        Vec<NodePointer> parents;
        if (last_sweep_start) {
          parents.reserve(nodes.size() - *last_sweep_start);
          for (size_t ix = *last_sweep_start; ix < nodes.size(); ++ix) {
            parents.emplace_back(ix);
          }
        } else {
          parents = initial_bases;
        }

        for (const auto &parent_pointer : parents) {
          const auto &[modifications, child_depth, earliest_rp] =
              visit_in_place(
                  parent_pointer,
                  [&](const NodePointerRoot &)
                      -> std::tuple<
                          ConstraintModifications,
                          size_t,
                          std::reference_wrapper<const RelayChainBlockInfo>> {
                    return std::make_tuple(ConstraintModifications{},
                                           size_t{0ull},
                                           scope.earliestRelayParent());
                  },
                  [&](const NodePointerStorage &ptr)
                      -> std::tuple<
                          ConstraintModifications,
                          size_t,
                          std::reference_wrapper<const RelayChainBlockInfo>> {
                    const auto &node = nodes[ptr];
                    if (auto opt_rcbi =
                            scope.ancestorByHash(node.relayParent())) {
                      return std::make_tuple(node.cumulative_modifications,
                                             size_t(node.depth + 1),
                                             opt_rcbi->get());
                    } else {
                      if (auto c = scope.getPendingAvailability(
                              node.candidate_hash)) {
                        return std::make_tuple(node.cumulative_modifications,
                                               size_t(node.depth + 1),
                                               c->get().relay_parent);
                      }
                      UNREACHABLE;
                    }
                  });

          if (child_depth > scope.max_depth) {
            continue;
          }

          auto child_constraints_res =
              scope.base_constraints.applyModifications(modifications);
          if (child_constraints_res.has_error()) {
            SL_TRACE(
                logger,
                "Failed to apply modifications. (error={}, new_parent_head={})",
                child_constraints_res.error().message(),
                modifications.required_parent);
            continue;
          }
          const auto &child_constraints = child_constraints_res.value();
          const auto required_head_hash =
              crypto::Hashed<const HeadData &, 32>{
                  child_constraints.required_parent}
                  .getHash();

          storage.iterParaChildren(
              required_head_hash, [&](const CandidateEntry &candidate) {
                auto pending =
                    scope.getPendingAvailability(candidate.candidate_hash);
                Option<RelayChainBlockInfo> relay_parent_opt;
                if (pending) {
                  relay_parent_opt = pending->get().relay_parent;
                } else {
                  relay_parent_opt = utils::fromRefToOwn(
                      scope.ancestorByHash(candidate.relay_parent));
                }
                if (!relay_parent_opt) {
                  return;
                }
                auto &relay_parent = *relay_parent_opt;

                uint32_t min_relay_parent_number;
                if (pending) {
                  min_relay_parent_number = visit_in_place(
                      parent_pointer,
                      [&](const NodePointerStorage &) {
                        return earliest_rp.number;
                      },
                      [&](const NodePointerRoot &) {
                        return pending->get().relay_parent.number;
                      });
                } else {
                  min_relay_parent_number = std::max(
                      earliest_rp.number, scope.earliestRelayParent().number);
                }

                if (relay_parent.number < min_relay_parent_number) {
                  return;
                }

                if (nodeHasCandidateChild(parent_pointer,
                                          candidate.candidate_hash)) {
                  return;
                }

                auto constraints = child_constraints;
                if (pending) {
                  constraints.min_relay_parent_number =
                      pending->get().relay_parent.number;
                }

                Option<Fragment> f = Fragment::create();
                if (!f) {
                  SL_TRACE(logger,
                           "Failed to instantiate fragment. (relay parent={}, "
                           "candidate hash={})",
                           relay_parent.hash,
                           candidate.candidate_hash);
                  return;
                }

                Fragment &fragment = *f;
                ConstraintModifications cumulative_modifications =
                    modifications;
                cumulative_modifications.stack(
                    fragment.constraintModifications());

                insertNode(FragmentNode{
                    .parent = parent_pointer,
                    .fragment = fragment,
                    .candidate_hash = candidate.candidate_hash,
                    .depth = child_depth,
                    .cumulative_modifications = cumulative_modifications,
                    .children = {}});
              });
        }

        last_sweep_start = sweep_start;
      } while (true);
    }

    void addAndPopulate(const CandidateHash &hash,
                        const CandidateStorage &storage) {
      auto opt_candidate_entry = storage.get(hash);
      if (!opt_candidate_entry) {
        return;
      }

      const auto &candidate_entry = opt_candidate_entry->get();
      const auto &candidate_parent =
          candidate_entry.candidate.persisted_validation_data.parent_head;

      Vec<NodePointer> bases{};
      if (scope.base_constraints.required_parent == candidate_parent) {
        bases.emplace_back(NodePointerRoot{});
      }

      for (size_t ix = 0ull; ix < nodes.size(); ++ix) {
        const auto &n = nodes[ix];
        if (n.cumulative_modifications.required_parent
            && n.cumulative_modifications.required_parent.value()
                   == candidate_parent) {
          bases.emplace_back(ix);
        }
      }

      populateFromBases(storage, bases);
    }

    void insertNode(FragmentNode &&node) {
      const NodePointerStorage pointer{nodes.size()};
      const auto parent_pointer = node.parent;
      const auto &candidate_hash = node.candidate_hash;
      const auto max_depth = scope.max_depth;

      auto &bv = candidates[candidate_hash];
      if (bv.bits.size() == 0ull) {
        bv.bits.resize(max_depth + 1);
      }
      bv.bits[node.depth] = true;

      visit_in_place(
          parent_pointer,
          [&](const NodePointerRoot &) {
            if (nodes.empty()
                || is_type<NodePointerRoot>(nodes.back().parent)) {
              nodes.emplace_back(std::move(node));
            } else {
              nodes.insert(
                  std::find_if(nodes.begin(),
                               nodes.end(),
                               [](const auto &item) {
                                 return !is_type<NodePointerRoot>(item);
                               }),
                  std::move(node));
            }
          },
          [&](const NodePointerStorage &ptr) {
            nodes.emplace_back(std::move(node));
            nodes[ptr].children.emplace_back(pointer, candidate_hash);
          });
    }

    Option<NodePointer> nodeCandidateChild(
        const NodePointer &pointer, const CandidateHash &candidate_hash) const {
      return visit_in_place(
          pointer,
          [&](const NodePointerStorage &ptr) -> Option<NodePointer> {
            if (ptr < nodes.size()) {
              return nodes[ptr].candidateChild(candidate_hash);
            }
            return std::nullopt;
          },
          [&](const NodePointerRoot &) -> Option<NodePointer> {
            for (size_t ix = 0ull; ix < nodes.size(); ++ix) {
              const FragmentNode &n = nodes[ix];
              if (!is_type<NodePointerRoot>(n.parent)) {
                break;
              }
              if (n.candidate_hash != candidate_hash) {
                continue;
              }
              return ix;
            }
            return std::nullopt;
          });
    }

    bool nodeHasCandidateChild(const NodePointer &pointer,
                               const CandidateHash &candidate_hash) const {
      return nodeCandidateChild(pointer, candidate_hash).has_value();
    }

    bool pathContainsBackedOnlyCandidates(
        NodePointer parent_pointer,
        const CandidateStorage &candidate_storage) const {
      while (auto ptr = if_type<NodePointerStorage>(parent_pointer)) {
        const auto &node = nodes[ptr->get()];
        const auto &candidate_hash = node.candidate_hash;

        auto opt_entry = candidate_storage.get(candidate_hash);
        if (!opt_entry || opt_entry->get().state != CandidateState::Backed) {
          return false;
        }
        parent_pointer = node.parent;
      }
      return true;
    }

    Vec<size_t> hypotheticalDepths(const CandidateHash &hash,
                                   const HypotheticalCandidate &candidate,
                                   const CandidateStorage &candidate_storage,
                                   bool backed_in_path_only) const {
      if (!backed_in_path_only) {
        if (auto it = candidates.find(hash); it != candidates.end()) {
          Vec<size_t> res;
          for (size_t ix = 0; ix < it->second.bits.size(); ++ix) {
            if (it->second.bits[ix]) {
              res.emplace_back(ix);
            }
          }
          return res;
        }
      }

      const auto &crp = relayParent(candidate);
      auto candidate_relay_parent =
          [&]() -> Option<std::reference_wrapper<const RelayChainBlockInfo>> {
        if (scope.relay_parent.hash == crp) {
          return {{scope.relay_parent}};
        }
        if (auto it = scope.ancestors_by_hash.find(crp);
            it != scope.ancestors_by_hash.end()) {
          return {{it->second}};
        }
        return std::nullopt;
      }();

      if (!candidate_relay_parent) {
        return {};
      }

      const auto max_depth = scope.max_depth;
      BitVec depths;
      depths.bits.resize(max_depth + 1);

      auto process_parent_pointer = [&](const NodePointer &parent_pointer) {
        const auto &[modifications, child_depth, earliest_rp] = visit_in_place(
            parent_pointer,
            [&](const NodePointerRoot &)
                -> std::tuple<
                    ConstraintModifications,
                    size_t,
                    std::reference_wrapper<const RelayChainBlockInfo>> {
              return std::make_tuple(ConstraintModifications{},
                                     size_t{0ull},
                                     scope.earliestRelayParent());
            },
            [&](const NodePointerStorage &ptr)
                -> std::tuple<
                    ConstraintModifications,
                    size_t,
                    std::reference_wrapper<const RelayChainBlockInfo>> {
              const auto &node = nodes[ptr];
              if (auto opt_rcbi = scope.ancestorByHash(node.relayParent())) {
                return std::make_tuple(node.cumulative_modifications,
                                       size_t(node.depth + 1),
                                       opt_rcbi->get());
              } else {
                if (auto r =
                        scope.getPendingAvailability(node.candidate_hash)) {
                  return std::make_tuple(node.cumulative_modifications,
                                         size_t(node.depth + 1),
                                         scope.earliestRelayParent());
                }
                UNREACHABLE;
              }
            });

        if (child_depth > max_depth) {
          return;
        }

        if (earliest_rp.get().number > candidate_relay_parent->get().number) {
          return;
        }

        auto child_constraints_res =
            scope.base_constraints.applyModifications(modifications);
        if (child_constraints_res.has_error()) {
          SL_TRACE(
              logger,
              "Failed to apply modifications. (error={}, new_parent_head={})",
              child_constraints_res.error().message(),
              modifications.required_parent);
          return;
        }

        const auto &child_constraints = child_constraints_res.value();
        const auto parent_head_hash = parentHeadDataHash(candidate);

        /// TODO(iceseer): keep hashed object in constraints to avoid recalc
        if (parent_head_hash
            != crypto::Hashed<const HeadData &, 32>{child_constraints
                                                        .required_parent}
                   .getHash()) {
          return;
        }

        if (auto const value =
                if_type<const HypotheticalCandidateComplete>(candidate)) {
          /// TODO(iceseer): do validation
        }

        if (!backed_in_path_only
            || pathContainsBackedOnlyCandidates(parent_pointer,
                                                candidate_storage)) {
          depths.bits[child_depth] = true;
        }
      };

      process_parent_pointer(NodePointerRoot{});
      for (size_t ix = 0; ix < nodes.size(); ++ix) {
        process_parent_pointer(ix);
      }

      Vec<size_t> res;
      for (size_t ix = 0; ix < depths.bits.size(); ++ix) {
        if (depths.bits[ix]) {
          res.emplace_back(ix);
        }
      }
      return res;
    }
  };

}  // namespace kagome::parachain::fragment

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::fragment, Constraints::Error);
OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::fragment, CandidateStorage::Error);

#endif  // KAGOME_PARACHAIN_FRAGMENT_TREE
