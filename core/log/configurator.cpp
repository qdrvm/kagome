/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
          - name: offchain
          - name: authority
          - name: crypto
            children:
              - name: bip39
              - name: crypto_store
              - name: ed25519
          - name: consensus
            children:
              - name: babe
                children:
                  - name: babe_lottery
                  - name: block_executor
                  - name: block_validator
              - name: grandpa
                children:
                  - name: voting_round
          - name: runtime
            level: trace
            children:
              - name: runtime_api
              - name: host_api
                children:
                  - name: memory_extension
                  - name: io_extension
                  - name: crypto_extension
                  - name: storage_extension
                  - name: child_storage_extension
                  - name: offchain_extension
                  - name: misc_extension
              - name: binaryen
              - name: wavm
              - name: wasmedge
          - name: metrics
          - name: network
            children:
              - name: synchronizer
              - name: kagome_protocols
                children:
                  - name: sync_protocol
          - name: changes_trie
          - name: storage
            children:
              - name: trie
          - name: transactions
          - name: pubsub
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
                             boost::filesystem::path path)
      : ConfiguratorFromYAML(std::move(previous),
                             std::filesystem::path(path.string())) {}
}  // namespace kagome::log
