/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "libp2p/multi/converters/converter_utils.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"
#include "common/hexutil.hpp"


using libp2p::multi::converters::multiaddrToBytes;
using libp2p::multi::converters::bytesToMultiaddrString;
using kagome::common::Buffer;
using kagome::common::unhex;

#define VALID_STR_TO_BYTES(addr, bytes) { \
auto res = multiaddrToBytes(addr); \
ASSERT_TRUE(res) << res.error().message(); \
Buffer sample = Buffer::fromHex(bytes).getValue(); \
ASSERT_EQ(res.value(),  sample); \
}

#define VALID_BYTES_TO_STR(addr, bytes) { \
auto res = bytesToMultiaddrString(Buffer{unhex(bytes).getValueRef()}); \
ASSERT_TRUE(res) << res.error().message(); \
ASSERT_EQ(res.value(),  addr); \
}

/**
 * @given a multiaddr
 * @when converting it to hex string representing multiaddr byte representation
 * @then if the supplied address was valid, a valid hex string is returned
 */
TEST(AddressConverter, StringToBytes) {
    VALID_STR_TO_BYTES("/ip4/1.2.3.4", "0401020304")
    VALID_STR_TO_BYTES("/ip4/0.0.0.0", "0400000000")
    VALID_STR_TO_BYTES("/udp/0", "91020000")
    VALID_STR_TO_BYTES("/tcp/0", "060000")
    VALID_STR_TO_BYTES("/udp/1234", "910204D2")
    VALID_STR_TO_BYTES("/tcp/1234", "0604D2")
    VALID_STR_TO_BYTES("/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/1234",
                       "A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B0604D2");
    VALID_STR_TO_BYTES(
            "/ip4/127.0.0.1/udp/1234/",
            "047F000001910204D2")
    VALID_STR_TO_BYTES("/ip4/127.0.0.1/udp/0/", "047F00000191020000")
    VALID_STR_TO_BYTES(
            "/ip4/127.0.0.1/tcp/1234/",
            "047F0000010604D2")
    VALID_STR_TO_BYTES(
            "/ip4/127.0.0.1/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/",
            "047F000001A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B")
    VALID_STR_TO_BYTES(
            "/ip4/127.0.0.1/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/1234/",
            "047F000001A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B0604D2")
}



/**
 * @given a byte array with its content representing a multiaddr
 * @when converting it to a multiaddr human-readable string
 * @then if the supplied byte sequence was valid, a valid multiaddr string is returned
 */
TEST(AddressConverter, BytesToString) {
    VALID_BYTES_TO_STR("/ip4/1.2.3.4/", "0401020304")
    VALID_BYTES_TO_STR("/ip4/0.0.0.0/", "0400000000")
    VALID_BYTES_TO_STR("/udp/0/", "91020000")
    VALID_BYTES_TO_STR("/tcp/0/", "060000")
    VALID_BYTES_TO_STR("/udp/1234/", "910204D2")
    VALID_BYTES_TO_STR("/tcp/1234/", "0604D2")
    VALID_BYTES_TO_STR("/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/1234/",
                       "A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B0604D2")
    VALID_BYTES_TO_STR(
            "/ip4/127.0.0.1/udp/1234/",
            "047F000001910204D2")
    VALID_BYTES_TO_STR("/ip4/127.0.0.1/udp/0/", "047F00000191020000")
    VALID_BYTES_TO_STR(
            "/ip4/127.0.0.1/tcp/1234/udp/0/udp/1234/",
            "047F0000010604D291020000910204D2")
    VALID_BYTES_TO_STR(
            "/ip4/127.0.0.1/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/",
            "047F000001A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B")
    VALID_BYTES_TO_STR(
            "/ip4/127.0.0.1/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/1234/",
            "047F000001A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B0604D2")
}
