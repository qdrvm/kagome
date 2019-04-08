/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multistream.hpp"
#include <gtest/gtest.h>
#include <functional>
#include "common/hexutil.hpp"
#include "libp2p/multi/utils/uvarint.hpp"

using kagome::common::Buffer;
using kagome::expected::Error;
using kagome::expected::Value;
using libp2p::multi::Multistream;
using libp2p::multi::UVarint;

/**
 * @given a protocol description and a buffer with data
 * @when creating a multistream object
 * @then a multistream object containing the protocol info and the data is
 * created
 */
TEST(Multistream, Create) {
  Multistream m =
      Multistream::create("/bittorrent.org/1.0", Buffer({1, 2, 3, 4}))
          .getValue();

  ASSERT_EQ(m.getProtocolPath(), "/bittorrent.org/1.0");
  ASSERT_EQ(Buffer({1, 2, 3, 4}), m.getEncodedData());

  Multistream m1 = m;

  ASSERT_EQ(m1.getProtocolPath(), "/bittorrent.org/1.0");
  ASSERT_EQ(Buffer({1, 2, 3, 4}), m.getEncodedData());
}

/**
 * @given a buffer with bytes, which is a valid representation of a multistream
 * @when creating a multistream object
 * @then a multistream object is created from data in the buffer
 */
TEST(Multistream, CreateFromBuffer) {
  std::string protocol =
      "/ipfs/Qmaa4Rw81a3a1VEx4LxB7HADUAXvZFhCoRdBzsMZyZmqHD/ipfs.protocol\n";
  Buffer buf;
  buf.put(UVarint(protocol.size() + 5).toBytes());
  buf.put(protocol);
  buf.put({1, 2, 3, 4, 5});
  Multistream m2 = Multistream::create(buf).getValue();

  ASSERT_EQ(
      m2.getProtocolPath(),
      "/ipfs/Qmaa4Rw81a3a1VEx4LxB7HADUAXvZFhCoRdBzsMZyZmqHD/ipfs.protocol");
  ASSERT_EQ(Buffer({1, 2, 3, 4, 5}), m2.getEncodedData());
  ASSERT_EQ(buf, m2.getBuffer());
}

/**
 * @given a buffer with bytes, which is a no valid representation of a
 * multistream
 * @when creating a multistream object
 * @then a multistream object is not created, an error is returned
 */
TEST(Multistream, FailCreate) {
  std::string protocol =
      "/ipfs/Qmaa4Rw81\na3a1VEx4LxB7HADUAXvZFhCoRdBzsMZyZ\nmqHD/"
      "ipfs.protocol\n";
  Buffer buf;
  buf.put({1, 2, 3, 4, 5});
  Multistream::create(protocol, buf)
      .match(
          [](Value<Multistream> &v) {
            ADD_FAILURE()
                << "Protocol was invalid, multistream shouldn't be created";
          },
          [](Error<std::string> &e) { ; });
}

/**
 * @given a multistream
 * @when adding a prefix to its path
 * @then path contains the prefix in the beginning, if the prefix was valid
 * returns Error if the condition was not satisfied
 */
TEST(Multistream, AddPrefix) {
  Multistream m = Multistream::create("/json", Buffer{1, 2, 3}).getValue();
  ASSERT_FALSE(m.addPrefix("/http/").hasValue());
  ASSERT_FALSE(m.addPrefix("ht\ntp").hasValue());
  ASSERT_TRUE(m.addPrefix("http").hasValue());
  ASSERT_EQ(m.getProtocolPath(), "/http/json");
  ASSERT_EQ(Buffer({1, 2, 3}), m.getEncodedData());
}

/**
 * @given a multistream
 * @when removing a prefix from its path
 * @then path does not contain the prefix, if it did and it is not empty after
 * removing the prefix returns Error if the condition was not satisfied
 */
TEST(Multistream, RmPrefix) {
  Multistream m = Multistream::create("/json", Buffer{1, 2, 3}).getValue();
  ASSERT_FALSE(m.removePrefix("/http").hasValue());
  ASSERT_FALSE(m.removePrefix("/json").hasValue());
  ASSERT_FALSE(m.removePrefix("json\n").hasValue());
  ASSERT_FALSE(m.removePrefix("json").hasValue());
  ASSERT_TRUE(m.addPrefix("html").hasValue());
  ASSERT_EQ(m.getProtocolPath(), "/html/json");
  ASSERT_TRUE(m.removePrefix("json").hasValue());
  ASSERT_EQ(m.getProtocolPath(), "/html");
  ASSERT_EQ(Buffer({1, 2, 3}), m.getEncodedData());
}
