/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/light.hpp"

#include "blockchain/block_header_repository.hpp"
#include "network/common.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/module_repository.hpp"
#include "storage/trie/on_read.hpp"

namespace kagome::network {
  LightProtocol::LightProtocol(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      const blockchain::GenesisBlockHash &genesis,
      std::shared_ptr<blockchain::BlockHeaderRepository> repository,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<runtime::ModuleRepository> module_repo,
      std::shared_ptr<runtime::Executor> executor,
      std::shared_ptr<runtime::RuntimeContextFactory> ctx_factory)
      : RequestResponseProtocolType{
          kName,
          host,
          make_protocols(kLightProtocol, genesis, chain_spec),
          log::createLogger(kName),
      },
      repository_{std::move(repository)},
      storage_{std::move(storage)},
      module_repo_{std::move(module_repo)},
      executor_{std::move(executor)},
      ctx_factory_(std::move(ctx_factory)) {}

  std::optional<outcome::result<LightProtocol::ResponseType>>
  LightProtocol::onRxRequest(RequestType req, std::shared_ptr<Stream>) {
    storage::trie::OnRead proof;
    OUTCOME_TRY(header, repository_->getBlockHeader(req.block));
    OUTCOME_TRY(
        batch,
        storage_->getProofReaderBatchAt(header.state_root, proof.onRead()));
    auto call = boost::get<LightProtocolRequest::Call>(&req.op);
    if (call) {
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({req.block, header.number},
                                              header.state_root));
      OUTCOME_TRY(
          ctx,
          ctx_factory_->fromBatch(
              instance,
              std::shared_ptr<storage::trie::TrieBatch>{std::move(batch)}));
      OUTCOME_TRY(executor_->callWithCtx(ctx, call->method, call->args));
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
