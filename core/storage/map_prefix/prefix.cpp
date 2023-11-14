/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/map_prefix/prefix.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <libp2p/common/bytestr.hpp>

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
    OUTCOME_TRY(cursor->seek(map._key(key)));
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
    return batch->put(map._key(key), std::move(value));
  }

  outcome::result<void> MapPrefix::Batch::remove(const BufferView &key) {
    return batch->remove(map._key(key));
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

  MapPrefix::MapPrefix(std::string_view prefix,
                       std::shared_ptr<BufferStorage> map)
      : MapPrefix{libp2p::bytestr(prefix), std::move(map)} {}

  Buffer MapPrefix::_key(BufferView key) const {
    return Buffer{prefix}.put(key);
  }

  outcome::result<BufferOrView> MapPrefix::get(const BufferView &key) const {
    return map->get(_key(key));
  }

  outcome::result<std::optional<BufferOrView>> MapPrefix::tryGet(
      const BufferView &key) const {
    return map->tryGet(_key(key));
  }

  outcome::result<bool> MapPrefix::contains(const BufferView &key) const {
    return map->contains(_key(key));
  }

  bool MapPrefix::empty() const {
    throw std::logic_error{"MapPrefix::empty not implemented"};
  }

  outcome::result<void> MapPrefix::put(const BufferView &key,
                                       BufferOrView &&value) {
    return map->put(_key(key), std::move(value));
  }

  outcome::result<void> MapPrefix::remove(const BufferView &key) {
    return map->remove(_key(key));
  }

  std::unique_ptr<BufferBatch> MapPrefix::batch() {
    return std::make_unique<Batch>(*this, map->batch());
  }

  std::unique_ptr<BufferStorageCursor> MapPrefix::cursor() {
    return std::make_unique<Cursor>(*this, map->cursor());
  }
}  // namespace kagome::storage
