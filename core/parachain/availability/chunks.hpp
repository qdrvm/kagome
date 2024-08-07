/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ec-cpp/ec-cpp.hpp>

#include "parachain/availability/erasure_coding_error.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

#define _EC_CPP_TRY_VOID(tmp, expr)                                            \
  auto &&tmp = expr;                                                           \
  if (ec_cpp::resultHasError(tmp)) {                                           \
    return kagome::ErasureCodingError(                                         \
        kagome::toErasureCodingError(ec_cpp::resultGetError(std::move(tmp)))); \
  }
#define _EC_CPP_TRY_OUT(tmp, out, expr) \
  _EC_CPP_TRY_VOID(tmp, expr);          \
  auto out = ec_cpp::resultGetValue(std::move(tmp));
#define EC_CPP_TRY(out, expr) _EC_CPP_TRY_OUT(OUTCOME_UNIQUE, out, expr)

namespace kagome::parachain {
  inline outcome::result<size_t> minChunks(size_t validators) {
    EC_CPP_TRY(min, ec_cpp::getRecoveryThreshold(validators));
    return min;
  }

  inline outcome::result<std::vector<network::ErasureChunk>> toChunks(
      size_t validators, const runtime::AvailableData &data) {
    OUTCOME_TRY(message, scale::encode(data));

    EC_CPP_TRY(encoder, ec_cpp::create(validators));
    EC_CPP_TRY(
        shards,
        encoder.encode(ec_cpp::Slice<uint8_t>(message.data(), message.size())));
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
    EC_CPP_TRY(encoder, ec_cpp::create(validators));
    std::vector<decltype(encoder)::Shard> _chunks;
    _chunks.resize(validators);
    for (size_t i = 0; i < chunks.size(); ++i) {
      const auto &chunk = chunks[i];
      if (chunk.index < validators) {
        _chunks[chunk.index] = chunk.chunk;
      }
    }

    EC_CPP_TRY(data, encoder.reconstruct(_chunks));
    return scale::decode<runtime::AvailableData>(data);
  }

  inline outcome::result<runtime::AvailableData> fromSystematicChunks(
      size_t validators, const std::vector<network::ErasureChunk> &chunks) {
    EC_CPP_TRY(encoder, ec_cpp::create(validators));
    std::vector<decltype(encoder)::Shard> _chunks;
    _chunks.resize(chunks.size());
    for (auto chunk : chunks) {
      _chunks[chunk.index] = chunk.chunk;
    }

    EC_CPP_TRY(data, encoder.reconstruct_from_systematic(_chunks));
    return scale::decode<runtime::AvailableData>(data);
  }
}  // namespace kagome::parachain

#undef ERASURE_CODING_ERROR
