/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/map_prefix/prefix.hpp"

#include <boost/algorithm/string/predicate.hpp>

namespace kagome::storage {
  inline std::optional<Buffer> afterPrefix(Buffer key) {
    auto carry = 1u;
    for (auto it{key.rbegin()}; (carry != 0) and it != key.rend(); ++it) {
      carry += *it;
      *it = carry & 0xFF;
      carry >>= 8;
    }
    if (carry != 0) {
      return std::nullopt;
    }
    return key;
  }

  MapPrefix::Cursor::Cursor(MapPrefix &map,
                            std::unique_ptr<BufferStorageCursor> cursor)
      : map{map}, cursor{std::move(cursor)} {}

  outcome::result<bool> MapPrefix::Cursor::seekFirst() {
    OUTCOME_TRY(cursor->seek(map.prefix));
    return isValid();
  }

  outcome::result<bool> MapPrefix::Cursor::seek(const BufferView &key) {
    OUTCOME_TRY(cursor->seek(map.prefix_key(key)));
    return isValid();
  }

  outcome::result<bool> MapPrefix::Cursor::seekLowerBound(
      const BufferView &key) {
    OUTCOME_TRY(cursor->seekLowerBound(map.prefix_key(key)));
    return isValid();
  }

  outcome::result<bool> MapPrefix::Cursor::seekLast() {
    if (map.after_prefix) {
      OUTCOME_TRY(cursor->seek(*map.after_prefix));
      if (cursor->isValid()) {
        OUTCOME_TRY(cursor->prev());
        return isValid();
      }
    }
    OUTCOME_TRY(cursor->seekLast());
    return isValid();
  }

  bool MapPrefix::Cursor::isValid() const {
    if (cursor->isValid()) {
      return startsWith(cursor->key().value(), map.prefix);
    }
    return false;
  }

  outcome::result<void> MapPrefix::Cursor::next() {
    assert(isValid());
    return cursor->next();
  }

  outcome::result<void> MapPrefix::Cursor::prev() {
    assert(isValid());
    return cursor->prev();
  }

  std::optional<Buffer> MapPrefix::Cursor::key() const {
    if (isValid()) {
      return cursor->key()->subbuffer(map.prefix.size());
    }
    return std::nullopt;
  }

  std::optional<BufferOrView> MapPrefix::Cursor::value() const {
    if (isValid()) {
      return cursor->value();
    }
    return std::nullopt;
  }

  MapPrefix::Batch::Batch(MapPrefix &map, std::unique_ptr<BufferBatch> batch)
      : map{map}, batch{std::move(batch)} {}

  outcome::result<void> MapPrefix::Batch::put(const BufferView &key,
                                              BufferOrView &&value) {
    return batch->put(map.prefix_key(key), std::move(value));
  }

  outcome::result<void> MapPrefix::Batch::remove(const BufferView &key) {
    return batch->remove(map.prefix_key(key));
  }

  outcome::result<void> MapPrefix::Batch::commit() {
    return batch->commit();
  }

  void MapPrefix::Batch::clear() {
    batch->clear();
  }

  MapPrefix::MapPrefix(BufferView prefix, std::shared_ptr<BufferStorage> map)
      : prefix{prefix},
        after_prefix{afterPrefix(this->prefix)},
        map{std::move(map)} {}

  Buffer MapPrefix::prefix_key(BufferView key) const {
    return Buffer{prefix}.put(key);
  }

  outcome::result<BufferOrView> MapPrefix::get(const BufferView &key) const {
    return map->get(prefix_key(key));
  }

  outcome::result<std::optional<BufferOrView>> MapPrefix::tryGet(
      const BufferView &key) const {
    return map->tryGet(prefix_key(key));
  }

  outcome::result<bool> MapPrefix::contains(const BufferView &key) const {
    return map->contains(prefix_key(key));
  }

  outcome::result<void> MapPrefix::put(const BufferView &key,
                                       BufferOrView &&value) {
    return map->put(prefix_key(key), std::move(value));
  }

  outcome::result<void> MapPrefix::remove(const BufferView &key) {
    return map->remove(prefix_key(key));
  }

  outcome::result<void> MapPrefix::removePrefix(const BufferView &prefix) {
    // it makes a prefixed prefix, yes
    return map->remove(prefix_key(prefix));
  }

  std::unique_ptr<BufferBatch> MapPrefix::batch() {
    return std::make_unique<Batch>(*this, map->batch());
  }

  std::unique_ptr<BufferStorageCursor> MapPrefix::cursor() {
    return std::make_unique<Cursor>(*this, map->cursor());
  }

  outcome::result<void> MapPrefix::clear() {
    return map->removePrefix(prefix);
  }

}  // namespace kagome::storage
