/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_IMPL_PROTOCOLS_LIGHT_HPP
#define KAGOME_NETWORK_IMPL_PROTOCOLS_LIGHT_HPP

#include "network/adapters/light.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"

namespace kagome::application {
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::primitives {
  struct GenesisBlockHeader;
}  // namespace kagome::primitives

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
      : public RequestResponseProtocol<LightProtocolRequest,
                                       LightProtocolResponse,
                                       ProtobufMessageReadWriter> {
    static constexpr auto kName = "LightProtocol";

   public:
    LightProtocol(libp2p::Host &host,
                  const application::ChainSpec &chain_spec,
                  const primitives::GenesisBlockHeader &genesis,
                  std::shared_ptr<blockchain::BlockHeaderRepository> repository,
                  std::shared_ptr<storage::trie::TrieStorage> storage,
                  std::shared_ptr<runtime::ModuleRepository> module_repo,
                  std::shared_ptr<runtime::Executor> executor,
                  std::shared_ptr<runtime::RuntimeContextFactory> ctx_factory);

    outcome::result<ResponseType> onRxRequest(RequestType req,
                                              std::shared_ptr<Stream>) override;

    void onTxRequest(const RequestType &) override {}

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> repository_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<runtime::ModuleRepository> module_repo_;
    std::shared_ptr<runtime::Executor> executor_;
    std::shared_ptr<runtime::RuntimeContextFactory> ctx_factory_;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_IMPL_PROTOCOLS_LIGHT_HPP
