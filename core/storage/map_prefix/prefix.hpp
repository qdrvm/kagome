/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/buffer_map_types.hpp"

namespace kagome::storage {
  struct MapPrefix : BufferStorage {
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

      MapPrefix &map;
      std::unique_ptr<BufferBatch> batch;
    };

    MapPrefix(BufferView prefix, std::shared_ptr<BufferStorage> map);
    MapPrefix(std::string_view prefix, std::shared_ptr<BufferStorage> map);
    Buffer _key(BufferView key) const;

    outcome::result<bool> contains(const BufferView &key) const override;
    bool empty() const override;
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
    std::shared_ptr<BufferStorage> map;
  };
}  // namespace kagome::storage
