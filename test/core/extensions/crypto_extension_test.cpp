/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <algorithm>

#include <gtest/gtest.h>
#include <gsl/span>
#include "core/runtime/mock_memory.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "mock/core/crypto/crypto_store_mock.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::extensions;
using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::crypto::Bip39Provider;
using kagome::crypto::Bip39ProviderImpl;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CryptoStore;
using kagome::crypto::CryptoStoreImpl;
using kagome::crypto::CryptoStoreMock;
using kagome::crypto::CSPRNG;
using kagome::crypto::ED25519Keypair;
using kagome::crypto::ED25519PrivateKey;
using kagome::crypto::ED25519Provider;
using kagome::crypto::ED25519ProviderImpl;
using kagome::crypto::ED25519PublicKey;
using kagome::crypto::ED25519Signature;
using kagome::crypto::Hasher;
using kagome::crypto::HasherImpl;
using kagome::crypto::Pbkdf2Provider;
using kagome::crypto::Pbkdf2ProviderImpl;
using kagome::crypto::Secp256k1Provider;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::SR25519Keypair;
using kagome::crypto::SR25519Provider;
using kagome::crypto::SR25519ProviderImpl;
using kagome::crypto::SR25519PublicKey;
using kagome::crypto::SR25519SecretKey;
using kagome::crypto::SR25519Signature;
using kagome::crypto::secp256k1::CompressedPublicKey;
using kagome::crypto::secp256k1::ExpandedPublicKey;
using kagome::crypto::secp256k1::MessageHash;
using kagome::crypto::secp256k1::RSVSignature;
using kagome::runtime::MockMemory;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmResult;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;

using ::testing::Return;

namespace sr25519_constants = kagome::crypto::constants::sr25519;
namespace ed25519_constants = kagome::crypto::constants::ed25519;

// The point is that sr25519 signature can have many valid values
// so we can only verify whether it is correct
MATCHER_P3(VerifySr25519Signature,
           provider,
           msg,
           pubkey,
           "check if matched sr25519 signature is correct") {
  SR25519Signature signature{};
  std::copy_n(arg.begin(), SR25519Signature::size(), signature.begin());

  return static_cast<bool>(provider->verify(signature, msg, pubkey));
};

class CryptoExtensionTest : public ::testing::Test {
 public:
  void SetUp() override {
    memory_ = std::make_shared<MockMemory>();

    random_generator_ = std::make_shared<BoostRandomGenerator>();
    sr25519_provider_ =
        std::make_shared<SR25519ProviderImpl>(random_generator_);
    ed25519_provider_ = std::make_shared<ED25519ProviderImpl>();
    secp256k1_provider_ = std::make_shared<Secp256k1ProviderImpl>();
    hasher_ = std::make_shared<HasherImpl>();
    bip39_provider_ = std::make_shared<Bip39ProviderImpl>(
        std::make_shared<Pbkdf2ProviderImpl>());

    crypto_store_ = std::make_shared<CryptoStoreMock>();
    crypto_ext_ = std::make_shared<CryptoExtension>(memory_,
                                                    sr25519_provider_,
                                                    ed25519_provider_,
                                                    secp256k1_provider_,
                                                    hasher_,
                                                    crypto_store_,
                                                    bip39_provider_);

    auto seed_hex =
        "a4681403ba5b6a3f3bd0b0604ce439a78244c7d43b127ec35cd8325602dd47fd";
    seed = kagome::common::Blob<32>::fromHex(seed_hex).value();

    // scale-encoded string
    seed_buffer.put(kagome::scale::encode(Buffer(seed)).value());

    sr25519_keypair = sr25519_provider_->generateKeypair(seed);
    sr25519_signature = sr25519_provider_->sign(sr25519_keypair, input).value();

    ed25519_keypair = ed25519_provider_->generateKeypair(seed);
    ed25519_signature = ed25519_provider_->sign(ed25519_keypair, input).value();

    std::copy_n(secp_message_vector.begin(),
                secp_message_vector.size(),
                secp_message_hash.begin());
    std::copy_n(secp_public_key_bytes.begin(),
                secp_public_key_bytes.size(),
                secp_uncompressed_public_key.begin());
    std::copy_n(secp_public_key_compressed_bytes.begin(),
                secp_public_key_compressed_bytes.size(),
                secp_compressed_pyblic_key.begin());
    std::copy_n(secp_signature_bytes.begin(),
                secp_signature_bytes.size(),
                secp_signature.begin());

    EXPECT_OUTCOME_TRUE(tmp1,
                        kagome::scale::encode(secp_uncompressed_public_key));
    scale_encoded_secp_uncompressed_public_key
        .putUint8(0)  // 0 means 0-th index in <Value, Error> variant type
        .put(tmp1);   // value itself
    EXPECT_OUTCOME_TRUE(tmp2,
                        kagome::scale::encode(secp_compressed_pyblic_key));
    scale_encoded_secp_compressed_public_key
        .putUint8(0)  // 0 means 0-th index in <Value, Error> variant type
        .put(tmp2);   // value

    secp_error_result.putUint8(1).putUint8(2);

    ed_public_keys_result
        .putUint8(4)  // scale-encoded size // 1
        .put(ed25519_keypair.public_key);

    sr_public_keys_result
        .putUint8(4)  // scale-encoded size // 1
        .put(sr25519_keypair.public_key);

    ed_public_keys.emplace_back(ed25519_keypair.public_key);
    sr_public_keys.emplace_back(sr25519_keypair.public_key);
    ed_public_key_buffer.put(ed25519_keypair.public_key);
    sr_public_key_buffer.put(sr25519_keypair.public_key);

    ed25519_signature_result      // encoded optional value
        .putUint8(1)              // 1 means that value presents
        .put(ed25519_signature);  // the value itself

    sr25519_signature_result      // encoded optional value
        .putUint8(1)              // 1 means that value presents
        .put(sr25519_signature);  // the value itself

    signature_failure_result_buffer.putUint8(0);
  }

 protected:
  std::shared_ptr<MockMemory> memory_;
  std::shared_ptr<CSPRNG> random_generator_;
  std::shared_ptr<SR25519Provider> sr25519_provider_;
  std::shared_ptr<ED25519Provider> ed25519_provider_;
  std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
  std::shared_ptr<Hasher> hasher_;
  std::shared_ptr<CryptoStoreMock> crypto_store_;
  std::shared_ptr<CryptoExtension> crypto_ext_;
  std::shared_ptr<Bip39Provider> bip39_provider_;

  Buffer input{"6920616d2064617461"_unhex};

  SR25519Signature sr25519_signature{};
  SR25519Keypair sr25519_keypair{};
  ED25519Signature ed25519_signature{};
  ED25519Keypair ed25519_keypair{};

  Buffer blake2b_128_result{"de944c5c12e55ee9a07cf5bf4b674995"_unhex};

  Buffer blake2b_256_result{
      "ba67336efd6a3df3a70eeb757860763036785c182ff4cf587541a0068d09f5b2"_unhex};

  Buffer keccak_result{
      "65aac3ad8b88cb79396da4c8b6a8cb6b5b74b0f6534a3e4e5e8ad68658feccf4"_unhex};

  Buffer twox_input{"414243444546"_unhex};

  Buffer twox128_result{
      184, 65, 176, 250, 243, 129, 181, 3, 77, 82, 63, 150, 129, 221, 191, 251};

  Buffer twox256_result{184, 65,  176, 250, 243, 129, 181, 3,   77,  82, 63,
                        150, 129, 221, 191, 251, 33,  226, 149, 136, 6,  232,
                        81,  118, 200, 28,  69,  219, 120, 179, 208, 237};

  Buffer secp_public_key_bytes{
      "04e32df42865e97135acfb65f3bae71bdc86f4d49150ad6a440b6f15878109880a0a2b2667f7e725ceea70c673093bf67663e0312623c8e091b13cf2c0f11ef652"_hex2buf};
  Buffer secp_public_key_compressed_bytes{
      "02e32df42865e97135acfb65f3bae71bdc86f4d49150ad6a440b6f15878109880a"_hex2buf};
  Buffer secp_signature_bytes{
      "90f27b8b488db00b00606796d2987f6a5f59ae62ea05effe84fef5b8b0e549984a691139ad57a3f0b906637673aa2f63d1f55cb1a69199d4009eea23ceaddc9301"_hex2buf};
  Buffer secp_message_vector{
      "ce0677bb30baa8cf067c88db9811f4333d131bf8bcf12fe7065d211dce971008"_hex2buf};
  MessageHash secp_message_hash;
  Buffer secp_error_result;
  Buffer ed_public_keys_result;
  Buffer sr_public_keys_result;
  Buffer ed_public_key_buffer;
  Buffer sr_public_key_buffer;
  Buffer ed25519_signature_result;
  Buffer sr25519_signature_result;

  std::vector<ED25519PublicKey> ed_public_keys;
  std::vector<SR25519PublicKey> sr_public_keys;

  RSVSignature secp_signature{};
  ExpandedPublicKey secp_uncompressed_public_key{};
  CompressedPublicKey secp_compressed_pyblic_key{};
  Buffer scale_encoded_secp_uncompressed_public_key;
  Buffer scale_encoded_secp_compressed_public_key;
  Buffer signature_failure_result_buffer;
  Blob<32> seed;
  Buffer seed_buffer;
};

/**
 * @given initialized crypto extension @and data, which can be
 * blake2b_128-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2_128Valid) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(
      *memory_,
      storeBuffer(out_ptr, gsl::span<const uint8_t>(blake2b_128_result)))
      .Times(1);

  crypto_ext_->ext_blake2_128(data, size, out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be
 * blake2b_256-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2_256Valid) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(
      *memory_,
      storeBuffer(out_ptr, gsl::span<const uint8_t>(blake2b_256_result)))
      .Times(1);

  crypto_ext_->ext_blake2_256(data, size, out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be keccak-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, KeccakValid) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_,
              storeBuffer(out_ptr, gsl::span<const uint8_t>(keccak_result)))
      .Times(1);

  crypto_ext_->ext_keccak_256(data, size, out_ptr);
}

/**
 * @given initialized crypto extension @and ed25519-signed message
 * @when verifying signature of this message
 * @then verification is successful
 */
TEST_F(CryptoExtensionTest, Ed25519VerifySuccess) {
  EXPECT_OUTCOME_TRUE(keypair, ed25519_provider_->generateKeypair());
  EXPECT_OUTCOME_TRUE(signature, ed25519_provider_->sign(keypair, input));

  Buffer pubkey_buf(keypair.public_key);
  Buffer sig_buf(signature);

  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, ed25519_constants::PUBKEY_SIZE))
      .WillOnce(Return(pubkey_buf));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, ed25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(sig_buf));

  ASSERT_EQ(crypto_ext_->ext_ed25519_verify(
                input_data, input_size, sig_data_ptr, pub_key_data_ptr),
            0);
}

/**
 * @given initialized crypto extension @and incorrect ed25519 signature for
 some
 * message
 * @when verifying signature of this message
 * @then verification fails
 */
TEST_F(CryptoExtensionTest, Ed25519VerifyFailure) {
  EXPECT_OUTCOME_TRUE(keypair, ed25519_provider_->generateKeypair());
  ED25519Signature invalid_signature;
  invalid_signature.fill(0x11);

  Buffer pubkey_buf(keypair.public_key);
  Buffer invalid_sig_buf(invalid_signature);

  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, ed25519_constants::PUBKEY_SIZE))
      .WillOnce(Return(pubkey_buf));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, ed25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(invalid_sig_buf));

  ASSERT_EQ(crypto_ext_->ext_ed25519_verify(
                input_data, input_size, sig_data_ptr, pub_key_data_ptr),
            5);
}

/**
 * @given initialized crypto extension @and sr25519-signed message
 * @when verifying signature of this message
 * @then verification is successful
 */
TEST_F(CryptoExtensionTest, Sr25519VerifySuccess) {
  auto pub_key = gsl::span<uint8_t>(sr25519_keypair.public_key);
  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, sr25519_constants::PUBLIC_SIZE))
      .WillOnce(Return(Buffer(pub_key)));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, sr25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(Buffer(sr25519_signature)));

  ASSERT_EQ(crypto_ext_->ext_sr25519_verify(
                input_data, input_size, sig_data_ptr, pub_key_data_ptr),
            0);
}

/**
 * @given initialized crypto extension @and sr25519-signed message
 * @when verifying signature of this message
 * @then verification fails
 */
TEST_F(CryptoExtensionTest, Sr25519VerifyFailure) {
  auto pub_key = gsl::span<uint8_t>(sr25519_keypair.public_key);
  auto false_signature = Buffer(sr25519_signature);
  ++false_signature[0];
  ++false_signature[1];
  ++false_signature[2];
  ++false_signature[3];

  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, sr25519_constants::PUBLIC_SIZE))
      .WillOnce(Return(Buffer(pub_key)));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, sr25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(false_signature));

  ASSERT_EQ(crypto_ext_->ext_sr25519_verify(
                input_data, input_size, sig_data_ptr, pub_key_data_ptr),
            5);
}

/**
 * @given initialized crypto extensions @and some bytes
 * @when XX-hashing those bytes to get 16-byte hash
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox128) {
  WasmPointer twox_input_data = 0;
  WasmSize twox_input_size = twox_input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(twox_input_data, twox_input_size))
      .WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_,
              storeBuffer(out_ptr, gsl::span<const uint8_t>(twox128_result)))
      .Times(1);

  crypto_ext_->ext_twox_128(twox_input_data, twox_input_size, out_ptr);
}

/**
 * @given initialized crypto extensions @and some bytes
 * @when XX-hashing those bytes to get 32-byte hash
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox256) {
  WasmPointer twox_input_data = 0;
  WasmSize twox_input_size = twox_input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(twox_input_data, twox_input_size))
      .WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_,
              storeBuffer(out_ptr, gsl::span<const uint8_t>(twox256_result)))
      .Times(1);

  crypto_ext_->ext_twox_256(twox_input_data, twox_input_size, out_ptr);
}

/**
 * @given initialized crypto extensions @and secp256k1 signature and message
 * @when call recovery public secp256k1 uncompressed key
 * @then resulting public key is correct
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverUncompressedSuccess) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = WasmResult(20, 20).combine();
  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;

  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(Buffer(sig_input)));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(
                  scale_encoded_secp_uncompressed_public_key)))
      .WillOnce(Return(res));

  auto ptrsize = crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_v1(sig, msg);
  ASSERT_EQ(ptrsize, res);
}

/**
 * @given initialized crypto extensions
 * @and a damaged secp256k1 signature and message
 * @when call recovery public secp256k1 uncompressed key
 * @then error code is returned
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverUncompressedFailure) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = WasmResult(20, 20).combine();
  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;
  auto sig_buffer = Buffer(sig_input);
  sig_buffer[4] = 0;  // damage signature
  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(sig_buffer));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(secp_error_result)))
      .WillOnce(Return(res));

  auto ptrsize = crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_v1(sig, msg);
  ASSERT_EQ(ptrsize, res);
}

/**
 * @given initialized crypto extensions @and secp256k1 signature and message
 * @when call recovery public secp256k1 compressed key
 * @then resulting public key is correct
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverCompressed) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = WasmResult(20, 20).combine();
  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;

  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(Buffer(sig_input)));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(
                  scale_encoded_secp_compressed_public_key)))
      .WillOnce(Return(res));

  auto ptrsize =
      crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_compressed_v1(sig, msg);
  ASSERT_EQ(ptrsize, res);
}

/**
 * @given initialized crypto extensions
 * @and a damaged secp256k1 signature and message
 * @when call recovery public secp256k1 compressed key
 * @then error code is returned
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverCompressedFailure) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = WasmResult(20, 20).combine();

  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;
  Buffer sig_buffer(sig_input);
  sig_buffer[4] = 0;  // damage signature

  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(sig_buffer));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(secp_error_result)))
      .WillOnce(Return(res));

  auto ptrsize =
      crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_compressed_v1(sig, msg);
  ASSERT_EQ(ptrsize, res);
}

/**
 * @given initialized crypto extension, key type
 * @when call ext_ed25519_public_keys_v1 of crypto extension
 * @then we get serialized set of existing ed25519 keys
 */
TEST_F(CryptoExtensionTest, Ed25519GetPublicKeysSuccess) {
  WasmSpan res = WasmResult(1, 2).combine();

  auto key_type = kagome::crypto::key_types::kBabe;

  EXPECT_CALL(*crypto_store_, getEd25519PublicKeys(key_type))
      .WillOnce(Return(ed_public_keys));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(ed_public_keys_result)))
      .WillOnce(Return(res));

  ASSERT_EQ(crypto_ext_->ext_ed25519_public_keys_v1(key_type), res);
}

/**
 * @given initialized crypto extension, key type
 * @when call ext_sr25519_public_keys_v1 of crypto extension
 * @then we get serialized set of existing sr25519 keys
 */
TEST_F(CryptoExtensionTest, Sr25519GetPublicKeysSuccess) {
  WasmSpan res = WasmResult(1, 2).combine();

  auto key_type = kagome::crypto::key_types::kBabe;

  EXPECT_CALL(*crypto_store_, getSr25519PublicKeys(key_type))
      .WillOnce(Return(sr_public_keys));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(sr_public_keys_result)))
      .WillOnce(Return(res));

  ASSERT_EQ(crypto_ext_->ext_sr25519_public_keys_v1(key_type), res);
}

/**
 * @given initialized crypto extension, key type, public key value and message
 * @when call ext_ed25519_sign_v1 of crypto extension
 * @then we get a valid signature
 */
TEST_F(CryptoExtensionTest, Ed25519SignSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::key_types::kBabe;
  kagome::runtime::WasmPointer key = 2;
  auto msg = WasmResult(3, 4).combine();
  auto res = WasmResult(5, 6).combine();

  // load public key
  EXPECT_CALL(*memory_, loadN(2, ED25519PublicKey::size()))
      .WillOnce(Return(ed_public_key_buffer));
  // load message
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(input));

  EXPECT_CALL(*crypto_store_,
              findEd25519Keypair(key_type, ed25519_keypair.public_key))
      .WillOnce(Return(ed25519_keypair));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(ed25519_signature_result)))
      .WillOnce(Return(res));
  ASSERT_EQ(crypto_ext_->ext_ed25519_sign_v1(key_type, key, msg), res);
}

/**
 * @given initialized crypto extension,
 * key type, not existing public key and message
 * @when call ext_ed25519_sign_v1 of crypto extension
 * @then we get a valid serialized error
 */
TEST_F(CryptoExtensionTest, Ed25519SignFailure) {
  kagome::runtime::WasmSize key_type = kagome::crypto::key_types::kBabe;
  kagome::runtime::WasmPointer key = 2;
  auto msg = WasmResult(3, 4).combine();
  auto res = WasmResult(5, 6).combine();

  // load public key
  EXPECT_CALL(*memory_, loadN(2, ED25519PublicKey::size()))
      .WillOnce(Return(ed_public_key_buffer));
  // load message
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(input));

  EXPECT_CALL(*crypto_store_,
              findEd25519Keypair(key_type, ed25519_keypair.public_key))
      .WillOnce(Return(
          outcome::failure(kagome::crypto::CryptoStoreError::KEY_NOT_FOUND)));

  EXPECT_CALL(
      *memory_,
      storeBuffer(gsl::span<const uint8_t>(signature_failure_result_buffer)))
      .WillOnce(Return(res));
  ASSERT_EQ(crypto_ext_->ext_ed25519_sign_v1(key_type, key, msg), res);
}

/**
 * @given initialized crypto extension, key type, public key value and message
 * @when call ext_sr25519_sign_v1 of crypto extension
 * @then we get a valid signature
 */
TEST_F(CryptoExtensionTest, Sr25519SignSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::key_types::kBabe;
  kagome::runtime::WasmPointer key = 2;
  auto msg = WasmResult(3, 4).combine();
  auto res = WasmResult(5, 6).combine();

  // load public key
  EXPECT_CALL(*memory_, loadN(2, SR25519PublicKey::size()))
      .WillOnce(Return(sr_public_key_buffer));
  // load message
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(input));

  EXPECT_CALL(*crypto_store_,
              findSr25519Keypair(key_type, sr25519_keypair.public_key))
      .WillOnce(Return(sr25519_keypair));

  EXPECT_CALL(
      *memory_,
      storeBuffer(VerifySr25519Signature(sr25519_provider_,
                                         gsl::span<const uint8_t>(input),
                                         sr25519_keypair.public_key)))
      .WillOnce(Return(res));

  ASSERT_EQ(crypto_ext_->ext_sr25519_sign_v1(key_type, key, msg), res);
}

/**
 * @given initialized crypto extension,
 * key type, not existing public key and message
 * @when call ext_sr25519_sign_v1 of crypto extension
 * @then we get a valid serialized error
 */
TEST_F(CryptoExtensionTest, Sr25519SignFailure) {
  kagome::runtime::WasmSize key_type = kagome::crypto::key_types::kBabe;
  kagome::runtime::WasmPointer key = 2;
  auto msg = WasmResult(3, 4).combine();
  auto res = WasmResult(5, 6).combine();

  // load public key
  EXPECT_CALL(*memory_, loadN(2, SR25519PublicKey::size()))
      .WillOnce(Return(sr_public_key_buffer));
  // load message
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(input));

  EXPECT_CALL(*crypto_store_,
              findSr25519Keypair(key_type, sr25519_keypair.public_key))
      .WillOnce(Return(
          outcome::failure(kagome::crypto::CryptoStoreError::KEY_NOT_FOUND)));

  EXPECT_CALL(
      *memory_,
      storeBuffer(gsl::span<const uint8_t>(signature_failure_result_buffer)))
      .WillOnce(Return(res));
  ASSERT_EQ(crypto_ext_->ext_sr25519_sign_v1(key_type, key, msg), res);
}

/**
 * @given initialized crypto extension, key type and seed
 * @when call generate ed25519 keypair method of crypto extension
 * @then a new ed25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, DISABLED_Ed25519GenerateSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::key_types::kBabe;
  kagome::runtime::WasmPointer res = 2;
  auto seed = WasmResult(3, 4).combine();

  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(seed_buffer));

  ASSERT_EQ(res, crypto_ext_->ext_ed25519_generate_v1(key_type, seed));
}

/**
 * @given initialized crypto extension, key type and seed
 * @when call generate sr25519 keypair method of crypto extension
 * @then a new sr25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Sr25519GenerateSuccess) {}
