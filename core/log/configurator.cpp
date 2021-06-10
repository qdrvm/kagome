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
    thread: none
    color: false
    latency: 0
groups:
  - name: main
    sink: console
    level: debug
    is_fallback: true
    children:
      - name: libp2p
        level: off
      - name: kagome
        children:
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
                  - name: babe_synchronizer
                  - name: block_executor
                  - name: block_validator
              - name: grandpa
                children:
                  - name: voting_round
          - name: runtime
            children:
              - name: runtime_api
              - name: host_api
              - name: binaryen
          - name: metrics
          - name: network
          - name: changes_trie
          - name: storage
          - name: pubsub
      - name: others
        level: info
        children:
          - name: testing
            level: info
          - name: debug
            level: info
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
