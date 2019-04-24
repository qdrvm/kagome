/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFER_MAP_HPP
#define KAGOME_BUFFER_MAP_HPP

/**
 * This file contains:
 *  - BufferMap - contains key-value bindings of Buffers
 *  - PersistedBufferMap - stores key-value bindings on filesystem or remote
 * connection.
 */

#include <gsl/span>

#include "common/buffer.hpp"
#include "storage/concept/generic_map.hpp"
#include "storage/concept/persisted_map.hpp"
#include "storage/concept/write_batch.hpp"

namespace kagome::storage {

  using Buffer = common::Buffer;

  using BufferMap = concept ::GenericMap<Buffer, Buffer>;

  using BufferBatch = concept ::WriteBatch<Buffer, Buffer>;

  using PersistedBufferMap = concept ::PersistedMap<Buffer, Buffer>;

  using BufferMapCursor = concept ::MapCursor<Buffer, Buffer>;

}  // namespace kagome::storage

#endif  // KAGOME_BUFFER_MAP_HPP
