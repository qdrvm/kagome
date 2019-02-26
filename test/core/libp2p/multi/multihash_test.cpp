/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "common/hexutil.hpp"
#include "libp2p/multi/multihash.hpp"
#include "libp2p/multi/uvarint.hpp"

using kagome::common::hex;
using kagome::common::Buffer;
using kagome::expected::Error;
using kagome::expected::Result;
using kagome::expected::Value;

using libp2p::multi::Multihash;
using libp2p::multi::HashType;
using libp2p::multi::UVarint;


/**
 *   @given a buffer with a hash
 *   @when creating a multihash using the buffer
 *   @then a correct multihash object is created if the hash size is not greater
 *         than maximum length
 **/
TEST(Multihash, Create) {
  Buffer hash{2, 3, 4};
  Multihash::create(HashType::kBlake2s128, hash)
      .match(
          [&hash](Value<Multihash> m) {
            ASSERT_EQ(m.value.getType(), HashType::kBlake2s128);
            ASSERT_EQ(m.value.getHash(), hash);
          },
          [](auto e) { FAIL() << e.error; });

  Multihash::create(HashType::kBlake2s128, Buffer(200, 42))
      .match(
          [](Value<Multihash> m) {
            FAIL() << "The multihash mustn't accept hashes of the size greater "
                      "than 127";
          },
          [](auto e) { SUCCEED(); });
}

/**
 *   @given a buffer with a hash or a hex string with a hash
 *   @when creating a multihash from them
 *   @then a correct multihash object is created if the given hash object was
 *         valid, and the hex representation of the created multihash matches the given
 *         hash string
 **/
TEST(Multihash, FromToHex) {
  Buffer hash {2, 3, 4};

  Multihash::create(HashType::kBlake2s128, hash)
      .match(
          [&hash](Value<Multihash> m) {
            UVarint var(HashType::kBlake2s128);
            auto hex_s = hex(var.toBytes().data(), var.toBytes().size())+"03"+hex(hash.toVector());
            ASSERT_EQ(m.value.toHex(), hex_s); },
          [](auto e) { FAIL() << e.error; });
  Multihash::createFromHex("1203020304")
      .match(
          [](Value<Multihash> m) { ASSERT_EQ(m.value.toHex(), "1203020304"); },
          [](auto e) { FAIL() << e.error; });

  Multihash::createFromHex("32004324234234")
      .match([](Value<Multihash> m) { FAIL() << "The length mustn't be zero"; },
             [](auto e) { SUCCEED(); });
  Multihash::createFromHex("32034324234234")
      .match(
          [](Value<Multihash> m) {
            FAIL() << "The length must be equal to the hash size";
          },
          [](auto e) { SUCCEED(); });
  Multihash::createFromHex("3204abcdefgh")
      .match(
          [](Value<Multihash> m) { FAIL() << "The hex string is invalid"; },
          [](auto e) { SUCCEED(); });
}

/**
 *   @given a multihash or a buffer
 *   @when converting a multihash to a buffer or creating one from a buffer
 *   @then a correct multihash object is created if the hash size is not greater
 *         than maximum length or correct buffer object representing the multihash is returned
 **/
TEST(Multihash, FromToBuffer) {
  Buffer hash {0x82, 3, 2, 3, 4};

  Multihash::createFromBuffer(hash)
      .match(
          [&hash](Value<Multihash> m) {
            ASSERT_EQ(m.value.toBuffer(), hash);
            },
          [](auto e) { FAIL() << e.error; });

  Multihash::createFromBuffer({2, 3, 1, 3})
      .match(
          [](Value<Multihash> m) {
            FAIL() << "Length in the header does not equal actual length";
          },
          [](auto e) { SUCCEED(); });

}
