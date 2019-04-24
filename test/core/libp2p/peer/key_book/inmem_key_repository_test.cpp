/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include <gtest/gtest.h>

#include "libp2p/crypto/key_pair.hpp"
#include "libp2p/peer/inmem_key_repository/inmem_key_repository.hpp"
#include "libp2p/peer/key_repository.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::peer;
using namespace libp2p::multi;
using namespace kagome::common;
using namespace libp2p::crypto;

struct InmemKeyRepositoryTest : ::testing::Test {
  static PeerId createPeerId(HashType type, Buffer b) {
    auto r1 = PeerId::create(type, std::move(b));
    if (!r1) {
      throw std::invalid_argument(r1.error().message());
    }

    return r1.value();
  }

  PeerId p1 = createPeerId(HashType::sha256, {1});
  PeerId p2 = createPeerId(HashType::sha512, {2});

  std::unique_ptr<KeyRepository> db = std::make_unique<InmemKeyRepository>();
};

TEST_F(InmemKeyRepositoryTest, PubkeyStore) {
  db->addPublicKey(p1, {{Key::Type::ED25519, Buffer{'a'}}});
  db->addPublicKey(p1, {{Key::Type::ED25519, Buffer{'b'}}});
  // insert same pubkey. it should not be inserted
  db->addPublicKey(p1, {{Key::Type::ED25519, Buffer{'b'}}});
  // same pubkey but different type
  db->addPublicKey(p1, {{Key::Type::RSA1024, Buffer{'b'}}});
  // put pubkey to different peer
  db->addPublicKey(p2, {{Key::Type::RSA4096, Buffer{'c'}}});

  EXPECT_OUTCOME_TRUE(v, db->getPublicKeys(p1));
  EXPECT_EQ(v->size(), 3);

  db->clear(p1);

  EXPECT_EQ(v->size(), 0);
}

TEST_F(InmemKeyRepositoryTest, KeyPairStore) {
  PublicKey pub = {{Key::Type::RSA1024, Buffer{'a'}}};
  PrivateKey priv = {{Key::Type::RSA1024, Buffer{'b'}}};
  KeyPair kp{pub, priv};
  db->addKeyPair(p1, {pub, priv});

  EXPECT_OUTCOME_TRUE(v, db->getKeyPairs(p1));
  EXPECT_EQ(v->size(), 1);

  EXPECT_EQ(*v, std::unordered_set<KeyPair>{kp});
}
