/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/buffer_map_types.hpp"
#include "storage/face/batch_writeable.hpp"

namespace kagome::storage {
  /**
   * Map wrapper to use keys under prefix.
   * Cursor removes key prefix and can seeks first/last.
   */
  struct MapPrefix : BufferBatchableStorage {
    struct Cursor : BufferStorageCursor {
      Cursor(MapPrefix &map, std::unique_ptr<BufferStorageCursor> cursor);

      outcome::result<bool> seekFirst() override;
      outcome::result<bool> seek(const BufferView &key) override;
      outcome::result<bool> seekLast() override;
      bool isValid() const override;
      outcome::result<void> next() override;
      outcome::result<void> prev() override;
      std::optional<Buffer> key() const override;
      std::optional<BufferOrView> value() const override;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      MapPrefix &map;
      std::unique_ptr<BufferStorageCursor> cursor;
    };

    struct Batch : BufferBatch {
      Batch(MapPrefix &map, std::unique_ptr<BufferBatch> batch);

      outcome::result<void> put(const BufferView &key,
                                BufferOrView &&value) override;
      outcome::result<void> remove(const BufferView &key) override;
      outcome::result<void> commit() override;
      void clear() override;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      MapPrefix &map;
      std::unique_ptr<BufferBatch> batch;
    };

    MapPrefix(BufferView prefix, std::shared_ptr<BufferBatchableStorage> map);
    Buffer _key(BufferView key) const;

    outcome::result<bool> contains(const BufferView &key) const override;
    outcome::result<BufferOrView> get(const BufferView &key) const override;
    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override;
    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;
    outcome::result<void> remove(const BufferView &key) override;
    std::unique_ptr<BufferBatch> batch() override;
    std::unique_ptr<BufferStorageCursor> cursor() override;

    Buffer prefix;
    std::optional<Buffer> after_prefix;
    std::shared_ptr<BufferBatchableStorage> map;
  };
}  // namespace kagome::storage
