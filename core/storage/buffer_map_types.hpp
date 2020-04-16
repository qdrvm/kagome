/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
