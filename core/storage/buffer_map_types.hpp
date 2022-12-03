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
#include "common/buffer_or_view.hpp"
#include "storage/face/batch_writeable.hpp"
#include "storage/face/generic_maps.hpp"
#include "storage/face/write_batch.hpp"

namespace kagome::storage::face {
  template <>
  struct OwnedOrViewTrait<common::Buffer> {
    using type = common::BufferOrView;
  };
}  // namespace kagome::storage::face

namespace kagome::storage {

  using Buffer = common::SLBuffer<std::numeric_limits<size_t>::max()>;
  using BufferView = common::BufferView;
  using common::BufferOrView;

  using BufferBatch = face::WriteBatch<BufferView, Buffer>;

  using ReadOnlyBufferMap = face::ReadOnlyMap<BufferView, Buffer>;

  using BufferStorage = face::GenericStorage<Buffer, Buffer, BufferView>;

  using BufferMap = face::GenericMap<BufferView, Buffer>;

  using BufferMapCursor = face::MapCursor<BufferView, Buffer, BufferView>;

  using BufferStorageCursor = face::MapCursor<Buffer, Buffer, BufferView>;

}  // namespace kagome::storage

#endif  // KAGOME_BUFFER_MAP_TYPES_HPP
