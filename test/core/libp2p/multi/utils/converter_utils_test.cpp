/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "libp2p/multi/converters/converter_utils.hpp"
extern "C" {
#include "libp2p/multi/c-utils/protoutils.h"
}
using libp2p::multi::converters::intToHex;
using libp2p::multi::converters::hexToInt;
using kagome::common::Buffer;

TEST(ConverterUtils, IntToHex) {
  ASSERT_EQ(intToHex(12345), Int_To_Hex(12345));
  ASSERT_EQ(intToHex(0), Int_To_Hex(0));
  ASSERT_EQ(intToHex(443), Int_To_Hex(443));
  ASSERT_EQ(intToHex(-1231), Int_To_Hex(-1231));
}

TEST(ConverterUtils, HexToInt) {
  ASSERT_EQ(hexToInt("04"), 4);
  ASSERT_EQ(hexToInt("14"), 20);
}
