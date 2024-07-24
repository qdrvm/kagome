/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log/configurator.hpp"

namespace kagome::log {

  namespace {
    std::string embedded_config(R"(
# ----------------
sinks:
  - name: console
    type: console
    stream: stderr
    thread: name
    color: false
    latency: 0
groups:
  - name: main
    sink: console
    level: info
    is_fallback: true
    children:
      - name: libp2p
        level: off
      - name: kagome
        children:
          - name: profile
          - name: injector
          - name: application
          - name: rpc
            children:
            - name: rpc_transport
            - name: api
              children:
                - name: author_api
          - name: authorship
          - name: blockchain
            children:
              - name: block_tree
              - name: block_storage
              - name: digest_tracker
          - name: offchain
          - name: authority
          - name: crypto
            children:
              - name: bip39
              - name: key_store
              - name: ed25519
              - name: ecdsa
          - name: consensus
            children:
              - name: timeline
              - name: babe
                children:
                  - name: babe_lottery
                  - name: block_appender
                  - name: block_executor
                  - name: block_validator
                  - name: babe_config_repo
              - name: grandpa
                children:
                  - name: voting_round
          - name: parachain
            children:
             - name: pvf_executor
          - name: dispute
          - name: runtime
            children:
              - name: runtime_api
              - name: host_api
                children:
                  - name: elliptic_curves_extension
                  - name: memory_extension
                  - name: io_extension
                  - name: crypto_extension
                  - name: storage_extension
                  - name: child_storage_extension
                  - name: offchain_extension
                  - name: misc_extension
                  - name: runtime_cache
              - name: binaryen
              - name: wavm
              - name: wasmedge
          - name: metrics
          - name: telemetry
          - name: network
            children:
              - name: reputation
              - name: synchronizer
              - name: authority_discovery
              - name: kagome_protocols
                children:
                  - name: block_announce_protocol
                  - name: grandpa_protocol
                  - name: propagate_transactions_protocol
                  - name: sync_protocol
                  - name: state_protocol
                  - name: warp_sync_protocol
                  - name: parachain_protocols
                    children:
                      - name: collation_protocol_vstaging
                      - name: validation_protocol_vstaging
                      - name: req_collation_protocol
                      - name: req_chunk_protocol
                      - name: req_available_data_protocol
                      - name: req_statement_protocol
                      - name: req_pov_protocol
                      - name: dispute_protocol
                      - name: req_attested_candidate_protocol
          - name: changes_trie
          - name: storage
            children:
              - name: trie
              - name: trie_pruner
          - name: transactions
          - name: pubsub
          - name: threads
      - name: others
        children:
          - name: testing
          - name: debug
# ----------------
  )");
  }

  Configurator::Configurator(std::shared_ptr<PrevConfigurator> previous)
      : ConfiguratorFromYAML(std::move(previous), embedded_config) {}

  Configurator::Configurator(std::shared_ptr<PrevConfigurator> previous,
                             std::string config)
      : ConfiguratorFromYAML(std::move(previous), std::move(config)) {}

  Configurator::Configurator(std::shared_ptr<PrevConfigurator> previous,
                             filesystem::path path)
      : ConfiguratorFromYAML(std::move(previous),
                             filesystem::path(path.string())) {}
}  // namespace kagome::log
