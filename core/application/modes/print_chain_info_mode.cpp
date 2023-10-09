/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/modes/print_chain_info_mode.hpp"

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson {
  typedef ::std::size_t SizeType;
}
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <iostream>

#include "blockchain/block_tree.hpp"

namespace kagome::application::mode {
  using common::hex_lower_0x;

  int PrintChainInfoMode::run() const {
    auto &genesis_hash = block_tree_->getGenesisBlockHash();
    auto finalized = block_tree_->getLastFinalized();
    auto best = block_tree_->bestBlock();

    rapidjson::Document document;
    document.SetObject();
    auto &allocator = document.GetAllocator();
    auto str_val = [&](std::string_view str) {
      return rapidjson::Value(str.data(), str.size(), allocator);
    };
    document.AddMember(
        "genesis_hash", str_val(hex_lower_0x(genesis_hash)), allocator);
    document.AddMember(
        "finalized_hash", str_val(hex_lower_0x(finalized.hash)), allocator);
    document.AddMember("finalized_number", finalized.number, allocator);
    document.AddMember(
        "best_hash", str_val(hex_lower_0x(best.hash)), allocator);
    document.AddMember("best_number", best.number, allocator);

    rapidjson::OStreamWrapper stream = std::cout;
    rapidjson::Writer writer(stream);
    document.Accept(writer);
    std::cout << std::endl;

    return EXIT_SUCCESS;
  }
}  // namespace kagome::application::mode
