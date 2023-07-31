/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/light.hpp"

#include "network/common.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/runtime_environment_factory.hpp"
#include "storage/trie/on_read.hpp"

namespace kagome::network {
  LightProtocol::LightProtocol(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      const primitives::GenesisBlockHeader &genesis,
      std::shared_ptr<blockchain::BlockHeaderRepository> repository,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<runtime::RuntimeEnvironmentFactory> env_factory)
      : RequestResponseProtocolType{
          kName,
          host,
          make_protocols(kLightProtocol, genesis, chain_spec),
          log::createLogger(kName),
      },
      repository_{std::move(repository)},
      storage_{std::move(storage)},
      env_factory_{std::move(env_factory)} {}

  outcome::result<LightProtocol::ResponseType> LightProtocol::onRxRequest(
      RequestType req, std::shared_ptr<Stream>) {
    storage::trie::OnRead proof;
    OUTCOME_TRY(header, repository_->getBlockHeader(req.block));
    OUTCOME_TRY(
        batch,
        storage_->getProofReaderBatchAt(header.state_root, proof.onRead()));
    auto call = boost::get<LightProtocolRequest::Call>(&req.op);
    if (call) {
      OUTCOME_TRY(factory, env_factory_->start(req.block));
      OUTCOME_TRY(env, factory->withStorageBatch(std::move(batch)).make());
      OUTCOME_TRY(
          env->module_instance->callExportFunction(call->method, call->args));
      OUTCOME_TRY(env->module_instance->resetEnvironment());
    } else {
      auto &read = boost::get<LightProtocolRequest::Read>(req.op);
      runtime::TrieStorageProviderImpl provider{storage_, nullptr};
      provider.setTo(std::move(batch));
      std::reference_wrapper<const storage::trie::TrieBatch> trie =
          *provider.getCurrentBatch();
      if (read.child) {
        OUTCOME_TRY(child, provider.getChildBatchAt(*read.child));
        trie = child;
      }
      for (auto &key : read.keys) {
        OUTCOME_TRY(trie.get().tryGet(key));
      }
    }
    return LightProtocolResponse{proof.vec(), call != nullptr};
  }
}  // namespace kagome::network
