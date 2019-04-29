/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect.hpp"

#include <gtest/gtest.h>
#include "core/libp2p/transport_fixture/transport_fixture.hpp"

using libp2p::protocol_muxer::Multiselect;

class MultiselectTest : public libp2p::testing::TransportFixture {
 public:
  Multiselect multiselect_;
};

TEST_F(MultiselectTest, NegotiateSuccess) {}

TEST_F(MultiselectTest, NegotiateFailure) {}

TEST_F(MultiselectTest, NoProtocols) {}
