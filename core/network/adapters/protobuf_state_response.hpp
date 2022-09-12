/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_PROTOBUF_STATE_RESPONSE
#define KAGOME_ADAPTERS_PROTOBUF_STATE_RESPONSE

#include "network/adapters/protobuf.hpp"

#include "network/types/state_response.hpp"
#include "scale/scale.hpp"

namespace kagome::network {

  template <>
  struct ProtobufMessageAdapter<StateResponse> {
    static size_t size(const StateResponse &t) {
      return 0;
    }

    static std::vector<uint8_t>::iterator write(
        const StateResponse &t,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator loaded) {
      ::api::v1::StateResponse msg;
      for (const auto &entries : t.entries) {
        auto *dst_entries = msg.add_entries();
        dst_entries->set_state_root(entries.state_root.toString());
        for (const auto &entry : entries.entries) {
          auto *dst_entry = dst_entries->add_entries();
          dst_entry->set_key(entry.key.toString());
          dst_entry->set_value(entry.value.toString());
        }
        dst_entries->set_complete(entries.complete);
      }
      msg.set_proof(t.proof.toString());

      return appendToVec(msg, out, loaded);
    }

    static outcome::result<std::vector<uint8_t>::const_iterator> read(
        StateResponse &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      const auto remains = src.size() - std::distance(src.begin(), from);
      assert(remains >= size(out));

      ::api::v1::StateResponse msg;
      if (!msg.ParseFromArray(from.base(), remains))
        return AdaptersError::PARSE_FAILED;

      for (const auto &kvEntry : msg.entries()) {
        KeyValueStateEntry kv;
        auto root = kvEntry.state_root();
        if(!root.empty()) {
          kv.state_root = storage::trie::RootHash::fromString(root).value();
        }
        for (const auto &sEntry : kvEntry.entries()) {
          kv.entries.emplace_back(StateEntry{common::Buffer().put(sEntry.key()),
                                   common::Buffer().put(sEntry.value())});
        }
        kv.complete = kvEntry.complete();

        out.entries.emplace_back(std::move(kv));
      }
      out.proof = common::Buffer().put(msg.proof());

      std::advance(from, msg.ByteSizeLong());
      return from;
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF_STATE_RESPONSE
