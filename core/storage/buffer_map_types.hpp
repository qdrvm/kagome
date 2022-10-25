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

#include "common/buffer.hpp"
#include "storage/face/batch_writeable.hpp"
#include "storage/face/generic_maps.hpp"
#include "storage/face/write_batch.hpp"

namespace kagome::storage {

  using Buffer = common::BufferT<std::numeric_limits<size_t>::max()>;
  using BufferView = common::BufferView;
  using BufferConstRef = common::BufferConstRef;

  using BufferBatch = face::WriteBatch<BufferView, Buffer>;

  using ReadOnlyBufferMap = face::ReadOnlyMap<BufferView, Buffer>;

  using BufferStorage = face::GenericStorage<Buffer, Buffer, BufferView>;

  using BufferMap = face::GenericMap<BufferView, Buffer>;

  using BufferMapCursor =
      face::MapCursor<BufferView, BufferConstRef, BufferView>;

  using BufferStorageCursor = face::MapCursor<Buffer, Buffer, BufferView>;

}  // namespace kagome::storage

#endif  // KAGOME_BUFFER_MAP_TYPES_HPP
