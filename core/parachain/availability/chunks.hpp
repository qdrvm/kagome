/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ec-cpp/ec-cpp.hpp>

#include "parachain/availability/erasure_coding_error.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {
  inline outcome::result<size_t> minChunks(size_t validators) {
    auto res = ec_cpp::getRecoveryThreshold(validators);
    if (ec_cpp::resultHasError(res)) {
      return ErasureCodingError(
          toErasureCodingError(ec_cpp::resultGetError(std::move(res))));
    }
    return ec_cpp::resultGetValue(std::move(res));
  }

  inline outcome::result<std::vector<network::ErasureChunk>> toChunks(
      size_t validators, const runtime::AvailableData &data) {
    OUTCOME_TRY(message, ::scale::encode(data));

    auto create_result = ec_cpp::create(validators);
    if (ec_cpp::resultHasError(create_result)) {
      return ErasureCodingError(toErasureCodingError(
          ec_cpp::resultGetError(std::move(create_result))));
    }

    auto encoder = ec_cpp::resultGetValue(std::move(create_result));
    auto encode_result =
        encoder.encode(ec_cpp::Slice<uint8_t>(message.data(), message.size()));
    if (ec_cpp::resultHasError(encode_result)) {
      return ErasureCodingError(toErasureCodingError(
          ec_cpp::resultGetError(std::move(encode_result))));
    }

    auto shards = ec_cpp::resultGetValue(std::move(encode_result));
    BOOST_ASSERT(shards.size() == validators);

    std::vector<network::ErasureChunk> chunks;
    chunks.resize(validators);
    for (size_t i = 0; i < validators; ++i) {
      auto &chunk = chunks[i];
      chunk.index = i;
      chunk.chunk = std::move(shards[i]);
    }
    return chunks;
  }

  inline outcome::result<runtime::AvailableData> fromChunks(
      size_t validators, const std::vector<network::ErasureChunk> &chunks) {
    auto create_result = ec_cpp::create(validators);
    if (ec_cpp::resultHasError(create_result)) {
      return ErasureCodingError(toErasureCodingError(
          ec_cpp::resultGetError(std::move(create_result))));
    }

    auto encoder = ec_cpp::resultGetValue(std::move(create_result));
    std::vector<decltype(encoder)::Shard> _chunks;
    _chunks.resize(validators);
    for (size_t i = 0; i < chunks.size(); ++i) {
      const auto &chunk = chunks[i];
      if (chunk.index < validators) {
        _chunks[chunk.index] = chunk.chunk;
      }
    }

    auto reconstruct_result = encoder.reconstruct(_chunks);
    if (ec_cpp::resultHasError(reconstruct_result)) {
      return ErasureCodingError(toErasureCodingError(
          ec_cpp::resultGetError(std::move(reconstruct_result))));
    }
    auto data = ec_cpp::resultGetValue(std::move(reconstruct_result));
    return scale::decode<runtime::AvailableData>(data);
  }
}  // namespace kagome::parachain
