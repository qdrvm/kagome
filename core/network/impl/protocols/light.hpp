/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/adapters/light.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"

namespace kagome::application {
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::blockchain {
  class GenesisBlockHash;
}  // namespace kagome::blockchain

namespace kagome::runtime {
  class ModuleRepository;
  class Executor;
  class RuntimeContextFactory;
}  // namespace kagome::runtime

namespace kagome::storage::trie {
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::blockchain {
  class BlockHeaderRepository;
}  // namespace kagome::blockchain

namespace kagome::network {
  /**
   * https://github.com/paritytech/substrate/tree/master/client/network/light
   */
  class LightProtocol
      : public RequestResponseProtocolImpl<LightProtocolRequest,
                                           LightProtocolResponse,
                                           ProtobufMessageReadWriter> {
    static constexpr auto kName = "LightProtocol";

   public:
    LightProtocol(libp2p::Host &host,
                  const application::ChainSpec &chain_spec,
                  const blockchain::GenesisBlockHash &genesis,
                  std::shared_ptr<blockchain::BlockHeaderRepository> repository,
                  std::shared_ptr<storage::trie::TrieStorage> storage,
                  std::shared_ptr<runtime::ModuleRepository> module_repo,
                  std::shared_ptr<runtime::Executor> executor);

    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType req, std::shared_ptr<Stream>) override;

    void onTxRequest(const RequestType &) override {}

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> repository_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<runtime::ModuleRepository> module_repo_;
    std::shared_ptr<runtime::Executor> executor_;
  };
}  // namespace kagome::network
