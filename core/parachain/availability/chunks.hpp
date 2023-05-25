/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_CHUNKS_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_CHUNKS_HPP

#include <erasure_coding/erasure_coding.h>
#include <ec-cpp/ec-cpp.hpp>

#include "parachain/availability/erasure_coding_error.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {
  inline outcome::result<size_t> minChunks(size_t validators) {
    auto const res = ec_cpp::getRecoveryThreshold(10);

    unsigned long out = 0;
    auto r = ECCR_get_recovery_threshold(validators, &out);
    if (r.tag != NPRSResult_Tag::NPRS_RESULT_OK) {
      return ErasureCodingError(r.tag);
    }
    return out;
  }

  inline outcome::result<std::vector<network::ErasureChunk>> toChunks(
      size_t validators, const runtime::AvailableData &data) {
    OUTCOME_TRY(message, scale::encode(data));
    DataBlock _message{message.data(), message.size()};
    ChunksList _list{nullptr, 0};
    auto _free = gsl::finally([&] {
      if (_list.data) {
        ECCR_deallocate_chunk_list(&_list);
      }
    });
    auto r = ECCR_obtain_chunks(validators, &_message, &_list);
    if (r.tag != NPRSResult_Tag::NPRS_RESULT_OK) {
      return ErasureCodingError(r.tag);
    }
    assert(_list.count == validators);
    std::vector<network::ErasureChunk> chunks;
    chunks.resize(validators);
    for (size_t i = 0; i < validators; ++i) {
      auto &_chunk = _list.data[i];
      assert(_chunk.index == i);
      auto &chunk = chunks[i];
      chunk.index = i;
      chunk.chunk = common::BufferView(_chunk.data.array, _chunk.data.length);
    }
    return chunks;
  }

  inline outcome::result<runtime::AvailableData> fromChunks(
      size_t validators, const std::vector<network::ErasureChunk> &chunks) {
    std::vector<Chunk> _chunks;
    _chunks.resize(chunks.size());
    for (size_t i = 0; i < chunks.size(); ++i) {
      auto &_chunk = _chunks[i];
      auto &chunk = chunks[i];
      _chunk.index = chunk.index;
      _chunk.data = {
          const_cast<uint8_t *>(chunk.chunk.data()),
          chunk.chunk.size(),
      };
    }
    ChunksList _list{_chunks.data(), _chunks.size()};
    DataBlock _data{nullptr, 0};
    auto _free = gsl::finally([&] {
      if (_data.array) {
        ECCR_deallocate_data_block(&_data);
      }
    });
    auto r = ECCR_reconstruct(validators, &_list, &_data);
    if (r.tag != NPRSResult_Tag::NPRS_RESULT_OK) {
      return ErasureCodingError(r.tag);
    }
    return scale::decode<runtime::AvailableData>(
        common::BufferView(_data.array, _data.length));
  }
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_CHUNKS_HPP
