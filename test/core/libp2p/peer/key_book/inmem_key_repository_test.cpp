/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include <gtest/gtest.h>
#include "libp2p/crypto/key.hpp"
#include "libp2p/peer/key_repository/inmem_key_repository.hpp"
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

  PeerId p1_ = createPeerId(HashType::sha256, {1});
  PeerId p2_ = createPeerId(HashType::sha512, {2});

  std::unique_ptr<KeyRepository> db_ = std::make_unique<InmemKeyRepository>();
};

TEST_F(InmemKeyRepositoryTest, PubkeyStore) {
  db_->addPublicKey(p1_, {{Key::Type::ED25519, Buffer{'a'}}});
  db_->addPublicKey(p1_, {{Key::Type::ED25519, Buffer{'b'}}});
  // insert same pubkey. it should not be inserted
  db_->addPublicKey(p1_, {{Key::Type::ED25519, Buffer{'b'}}});
  // same pubkey but different type
  db_->addPublicKey(p1_, {{Key::Type::RSA1024, Buffer{'b'}}});
  // put pubkey to different peer
  db_->addPublicKey(p2_, {{Key::Type::RSA4096, Buffer{'c'}}});

  EXPECT_OUTCOME_TRUE(v, db_->getPublicKeys(p1_));
  EXPECT_EQ(v->size(), 3);

  db_->clear(p1_);

  EXPECT_EQ(v->size(), 0);
}

TEST_F(InmemKeyRepositoryTest, KeyPairStore) {
  PublicKey pub = {{Key::Type::RSA1024, Buffer{'a'}}};
  PrivateKey priv = {{Key::Type::RSA1024, Buffer{'b'}}};
  KeyPair kp{pub, priv};
  db_->addKeyPair(p1_, {pub, priv});

  EXPECT_OUTCOME_TRUE(v, db_->getKeyPairs(p1_));
  EXPECT_EQ(v->size(), 1);

  EXPECT_EQ(*v, std::unordered_set<KeyPair>{kp});
}

/**
 * @given 2 peers in storage
 * @when get peers
 * @then 2 peers returned
 */
TEST_F(InmemKeyRepositoryTest, GetPeers) {
  PublicKey z{};
  KeyPair kp{};

  db_->addPublicKey(p1_, z);
  db_->addKeyPair(p2_, kp);
  
  auto s = db_->getPeers();
  EXPECT_EQ(s.size(), 2);
}
