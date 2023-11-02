/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

  template <>
  struct ViewTrait<common::Buffer> {
    using type = common::BufferView;
  };
}  // namespace kagome::storage::face

namespace kagome::storage {
  using common::Buffer;
  using common::BufferOrView;
  using common::BufferView;

  using BufferBatch = face::WriteBatch<Buffer, Buffer>;

  using BufferStorage = face::GenericStorage<Buffer, Buffer>;

  using BufferStorageCursor = face::MapCursor<Buffer, Buffer>;
}  // namespace kagome::storage
