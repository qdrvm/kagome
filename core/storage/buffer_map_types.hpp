/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFER_MAP_TYPES_HPP
#define KAGOME_BUFFER_MAP_TYPES_HPP

/**
 * This file contains convenience typedefs for interfaces from face/, as they
 * are mostly used with Buffer key and value types
 */

#include <gsl/span>

#include "common/buffer.hpp"
#include "storage/face/batchable.hpp"
#include "storage/face/generic_storage.hpp"
#include "storage/face/write_batch.hpp"

namespace kagome::storage {

  using Buffer = common::Buffer;

  using BufferMap = face::GenericMap<Buffer, Buffer>;

  using BufferBatch = face::WriteBatch<Buffer, Buffer>;

  using ReadOnlyBufferMap = face::ReadOnlyMap<Buffer, Buffer>;
  using BatchWriteBufferMap = face::BatchWriteMap<Buffer, Buffer>;

  using BufferStorage = face::GenericStorage<Buffer, Buffer>;

  using BufferMapCursor = face::MapCursor<Buffer, Buffer>;

}  // namespace kagome::storage

#endif  // KAGOME_BUFFER_MAP_TYPES_HPP
