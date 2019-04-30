/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator/key_generator_impl.hpp"

#include <fstream>

#include <gtest/gtest.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <boost/filesystem.hpp>
#include <gsl/gsl_util>
#include "libp2p/crypto/random/impl/boost_generator.hpp"

using kagome::common::Buffer;
using libp2p::crypto::Key;
using libp2p::crypto::KeyGeneratorImpl;
using libp2p::crypto::common::RSAKeyType;
using libp2p::crypto::random::BoostRandomGenerator;

/// \brief contains keys data in string form
struct KeyData {
  std::string rsa_1024_private_key_ =
      "-----BEGIN RSA PRIVATE KEY-----\n"
      "Proc-Type: 4,ENCRYPTED\n"
      "DEK-Info: DES-EDE3-CBC,5ED390C9A32B4FCE\n"
      "\n"
      "ZotHAj4Pi1q0H/rG7Tjs36c5Xdnkr94C/JSCymTDy137uzq2wTrkK191p4L8zlPg\n"
      "AmgFgtvXl20/oW4JxMzRxvsvbyTic8ISvhh/U/jkqea+y+u1x8p7nvNfpHgWlV0u\n"
      "pQ48r8lCFtoCWOeCP1Jd4b4x+WVodzRgDt0mnzm1WHxxM9fNmQbPTrrU9YAYNTeB\n"
      "sAzS3JQgyG19opARkPsBGOsCVHV+DxFCn3uaxrZPYebywTkt/rjRi0TB5M9NKNdA\n"
      "bJ/T+BMyjBAWmSUl+d1a/KVhtBF5NyHk6GRg5sQ20vAy+CYnmkVTS/Ao2dQIXM+K\n"
      "O3GKDTOme2UIyK3+jiB0wvAJ9Zr4Kx7rotUxzS7OTnw0e/d1X9ur5x178opvuHh3\n"
      "rg2lSlmNoC3G9deOvit0YJDM6s7s6WrbswVCYCAhfeR6a7/VLdlvtLRo1dYfKV2c\n"
      "2ZKtmK5TwzVBglFrFjMglB6zmI03U54whz0/jSE7+P1AwjJbYFWFxbYywCjywh7A\n"
      "GKLzf+5YUVlz/8di/jWDpuRur7V04wfniKfKdnhjFMdw9xpN/x+Df40LT+bYYlFt\n"
      "Fpa8oZN5khMUtw33k0gM77Xb4P9FwGdX5S7Kt/+XMmopGb8fph5FxPO0pi8FCImd\n"
      "bXzaoa9nHW9iQAwHrffPols2TFUGPL2TPRWGhBXMjeK3IuLKrGlk4q3HB2ZNipPS\n"
      "UN4pmphclNR0rlc+d3wLhKtRpqWHBDdMqLpvQR6I/0G41oY4JF1Jhh46TgDpaHqq\n"
      "nF9i5Ahu3O0axAJE5eL2WEv9Sv3KK1kyt0NSReMj4eg=\n"
      "-----END RSA PRIVATE KEY-----";

  std::string rsa_2048_private_key_ =
      "-----BEGIN RSA PRIVATE KEY-----\n"
      "Proc-Type: 4,ENCRYPTED\n"
      "DEK-Info: DES-EDE3-CBC,22D2B75D5A46C83B\n"
      "\n"
      "RgsBCvdIoSfPWyMQpazlU5jBHxB5ez/vYAqTU3jbul11QstPzSOo9Vzkec8aej3L\n"
      "XgJ10/iuXpsuJ36MMNleF+Wc/l8PMevLrErCEpLDCW7uV39oYa02vBLaz6vhF+dA\n"
      "XFDxRwQ1J7fINPOBPnrEPFVowwBvL3Gwm92xltTBxhipuYX4HcZfTA482QP+zfD4\n"
      "cwl/OyVp2lB2wVqa7bedWRAPBThGzK5tzaWjb7QI2Zf2xYTzr2ND1CmNkeDlAOsk\n"
      "IfjMtDs7Wxsc3aDXA81tKRAFBxAMPv/QI+6YnQk7v/GrsFxEwT8W9LoBOCtVuSbR\n"
      "B1/UyCFBORXhhBo8BQHAhPZm3dL+obKbYekV46/xzNb1IBRfja+Oe2RwWT1rZyRy\n"
      "SdXJ2TphjteiKJdnVMZ6ZOWyCSeFbkRRAZMbZEmVcPQyKDVQpsIf+4de9+l8jNe6\n"
      "6z3Ga9UtdoCxr8sQUNxPk7BdaaYhlhSYpJl2KgJAXoLj2iorkxqbf0h/23EFINr/\n"
      "UcKWTzquH4kHMqjm0oM0DdQ9PjfBiHHPl+3Z/YBTh1j7vsykBzrF5YARjuEQYPZo\n"
      "T1pEsEKStQDpHDfWvCrlE5ZWWWrYOR2eY+Bz/bFokDMy0/HafBlyCItxQaVXigqw\n"
      "gn9GfertvuBDqCBALtTEKdY4UkFup1aJPY2rivdk/190I/7HGHHyGFK4OfRO1DBl\n"
      "51BldpPm969R8URoiR6PfVdrK9R48sRUE/VJWZB3r9Fx935uZdVGjbDz3h25OBYc\n"
      "kN43cr24GlfXbdyEEOdyRbkBU/ttgyV1sZzjy7MsVgniFV+I5PCjraJTAQgcfSVG\n"
      "gBlDH0Agk+HFukPvA3mRWub1N9zNFjeHfByF0QAIcWmCSpIraOKVZ3Kw9hyJOFg5\n"
      "LVODkNlgg5slWorahAnbXBlDrIzwhFtKWycchP/avvzIDXFqsA47YJYOZqJkSLvd\n"
      "ygCtP1kWvZDoktPWE9MXmjhAjTA5rwgDZHFyzRVicVhPPBUWqm21N9D534Xq0Ccj\n"
      "FLBvEK/KbAVUiquqRhommbmKjXNSsOExxJ/xAyL/tJxzr9hFXn15/9bvOaNfdjJm\n"
      "xzz9FjQnrzy2VkeDhAVQxOaFN9yof6/npDU4QsWjghZ6CQ3Mhnhv1GbPUtjf+uZE\n"
      "8+QZTCphShN7zdZmabOoRt3wX7AZkhb9eiuZHcyIUyZR7FmvK88ZtPjXZWl0ra2O\n"
      "H/LNam7NjrPs5rz9qtERzb3ntPQ2KaBsNP9CTv93Bs6A9TAgCQk9RoBbcM6NPUGk\n"
      "h3Gmw55VgcxhozHCgW1KjZuaeFfL+IpMnE6NRvZx1d82Et1tWwmfl8AY3f2pcTq6\n"
      "G1Qwv0rbkcUbeTBEHCJtGg5RO3N8haoSXr2x4rKzj0qQcMTl9w6gombu8xvIywly\n"
      "gKLjaWPbZOJx3kBdrS6NsVyOEOxfBbOHpHxCWZqVjCQjQSLB3dA6pX29aMnti3Ur\n"
      "CGkSlDco8zn+kgxNqvX/IKkB+j2EqJiO25FPkXq7Q7XW2x7s6NrGjyHMu56ons/e\n"
      "NPQbiqvbL5YKFs41r8a9R2unpxBYBEmVRyBgmMRBZelPplXxtd5CBeJp1Qn5e1Pt\n"
      "-----END RSA PRIVATE KEY-----";

  std::string rsa_4096_private_key_ =
      "-----BEGIN RSA PRIVATE KEY-----\n"
      "Proc-Type: 4,ENCRYPTED\n"
      "DEK-Info: DES-EDE3-CBC,33423F214AA4DEAC\n"
      "\n"
      "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\n"
      "-----END RSA PRIVATE KEY-----";

  std::string ed25519_private_key_ =
      "-----BEGIN PRIVATE KEY-----\n"
      "MC4CAQAwBQYDK2VwBCIEIBdcqTY4Ybi+EeJEOzX/GwsN5IYZp4pndoES2tb473I6\n"
      "-----END PRIVATE KEY-----";

  std::string secp256k1_private_key_ =
      "-----BEGIN EC PRIVATE KEY-----\n"
      "MHQCAQEEIDmfPdQEwSJiWRP/8nDQjUtRfVtLMp+9VddWbPB8d1UcoAcGBSuBBAAK\n"
      "oUQDQgAEflsDwKkQagCUyoo+luEyOD+07Evx6Sg4FkfmMoWNr5/ux2sKpC6379TN\n"
      "+R8zS1oIhsnnUZHJBUE5TN8yC108kQ==\n"
      "-----END EC PRIVATE KEY-----";
};

class KeyTest : public virtual testing::Test {
 protected:
  class TempFile {
   public:
    ~TempFile() {
      boost::filesystem::remove(path_);
    }

    boost::filesystem::path create(std::string_view data) {
      makeTempFile(data);
      return path_;
    }

    const auto &path() const {
      return path_;
    }

   private:
    void makeTempFile(std::string_view content) {
      path_ = boost::filesystem::unique_path().native();
      std::ofstream file{};
      file.open(path_.string(), std::ios::out);
      file << content << std::endl;
      file.close();
    }

    boost::filesystem::path path_;
  };

  BoostRandomGenerator random_;
  KeyGeneratorImpl keygen_{random_};
};

class GenerateKeyTest : public KeyTest {};

class ImportKeyTest : public KeyTest, public KeyData {};

class DeriveKeyTest : public KeyTest, public KeyData {};

/**
 * @given key generator
 * @when generateRsa of type RSA1024 is called
 * @then obtained key pair has type RSA1024
 */
TEST_F(GenerateKeyTest, generateRSA1024Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::RSA1024);
  ASSERT_EQ(val.publicKey.type, Key::Type::RSA1024);
}

/**
 * @given key generator
 * @when generateRsa of type RSA2048 is called
 * @then obtained key pair has type RSA2048
 */
TEST_F(GenerateKeyTest, generateRSA2048Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA2048);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::RSA2048);
  ASSERT_EQ(val.publicKey.type, Key::Type::RSA2048);
}

/**
 * @given key generator
 * @when generateRsa of type RSA4096 is called
 * @then obtained key pair has type RSA4096
 */
TEST_F(GenerateKeyTest, generateRSA4096Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA4096);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::RSA4096);
  ASSERT_EQ(val.publicKey.type, Key::Type::RSA4096);
}

/**
 * @given key generator instance
 * @when 2 keys of RSA1024 type are generated
 * @then these keys are different
 */
TEST_F(GenerateKeyTest, Rsa1024KeysNotSame) {
  auto &&res1 = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res1);
  auto &&val1 = res1.value();
  auto &&res2 = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res2);
  auto &&val2 = res2.value();
  ASSERT_NE(val1.privateKey.data.toVector(), val2.privateKey.data.toVector());
}

/**
 * @given key generator instance
 * @when generateEd25519 is called
 * @then obtained key pair have type ED25519
 */
TEST_F(GenerateKeyTest, generateED25519Success) {
  auto &&res = keygen_.generateEd25519();
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::ED25519);
  ASSERT_EQ(val.publicKey.type, Key::Type::ED25519);
}

/**
 * @given key generator instance
 * @when 2 keys of SECP256K1 type are generated
 * @then these keys are different
 */
TEST_F(GenerateKeyTest, Ed25519KeysNotSame) {
  auto &&res1 = keygen_.generateEd25519();
  ASSERT_TRUE(res1);
  auto &&val1 = res1.value();
  auto &&res2 = keygen_.generateEd25519();
  ASSERT_TRUE(res2);
  auto &&val2 = res2.value();
  ASSERT_NE(val1.privateKey.data.toVector(), val2.privateKey.data.toVector());
}

/**
 * @given key generator instance
 * @when generateSecp256k1 is called
 * @then obtained key pair have type SECP256K1
 */
TEST_F(GenerateKeyTest, generateSecp256k1Success) {
  auto &&res = keygen_.generateSecp256k1();
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::SECP256K1);
  ASSERT_EQ(val.publicKey.type, Key::Type::SECP256K1);
}

/**
 * @given key generator instance
 * @when 2 keys of SECP256K1 type are generated
 * @then these keys are different
 */
TEST_F(GenerateKeyTest, Secp256k1KeysNotSame) {
  auto &&res1 = keygen_.generateSecp256k1();
  ASSERT_TRUE(res1);
  auto &&val1 = res1.value();
  auto &&res2 = keygen_.generateSecp256k1();
  ASSERT_TRUE(res2);
  auto &&val2 = res2.value();
  ASSERT_NE(val1.privateKey.data.toVector(), val2.privateKey.data.toVector());
}

/**
 * @given RSA 1024 bits password-protected key file
 * @when importKey is called with correct password
 * @then operation succeeds
 */
TEST_F(ImportKeyTest, importRsa1024Successful) {
  TempFile key_file{};
  auto &&path = key_file.create(rsa_1024_private_key_);

  auto &&key = keygen_.importKey(path, "qweqweqwe");
  ASSERT_TRUE(key);
  auto &&val = key.value();
  ASSERT_EQ(val.type, Key::Type::RSA1024);
}

/**
 * @given RSA 1024 bits password-protected key file
 * @when importKey is called with wrong password
 * @then operation fails
 */
TEST_F(ImportKeyTest, importRsa1024Fail) {
  TempFile key_file{};
  auto &&path = key_file.create(rsa_1024_private_key_);
  auto &&key = keygen_.importKey(path, "asdasdasd");
  ASSERT_FALSE(key);
}

/**
 * @given RSA 2048 bits password-protected key file
 * @when importKey is called with correct password
 * @then operation succeeds
 */
TEST_F(ImportKeyTest, importRsa2048Successful) {
  TempFile key_file{};
  auto &&path = key_file.create(rsa_2048_private_key_);
  auto &&key = keygen_.importKey(path, "qweqweqwe");
  auto &&val = key.value();
  ASSERT_EQ(val.type, Key::Type::RSA2048);
}

/**
 * @given RSA 4096 bits password-protected key file
 * @when importKey is called with correct password
 * @then operation succeeds
 */
TEST_F(ImportKeyTest, importRsa4096Successful) {
  TempFile key_file{};
  auto &&path = key_file.create(rsa_4096_private_key_);
  auto &&key = keygen_.importKey(path, "qweqweqwe");
  auto &&val = key.value();
  ASSERT_EQ(val.type, Key::Type::RSA4096);
}

/**
 * @given ED25519 plain key file
 * @when importKey is called with empty password
 * @then operation succeeds
 */
TEST_F(ImportKeyTest, importEd25519Successful) {
  TempFile key_file{};
  auto &&path = key_file.create(ed25519_private_key_);
  auto &&key = keygen_.importKey(path, "");
  auto &&val = key.value();
  ASSERT_EQ(val.type, Key::Type::ED25519);
}

/**
 * @given Secp256k1 plain key file
 * @when importKey is called with empty password
 * @then operation succeeds
 */
TEST_F(ImportKeyTest, imporSecp256k1Successful) {
  TempFile key_file{};
  auto &&path = key_file.create(secp256k1_private_key_);
  auto &&key = keygen_.importKey(path, "");
  ASSERT_TRUE(key);
  auto &&val = key.value();
  ASSERT_EQ(val.type, Key::Type::SECP256K1);
}

/**
 * @given RSA 1024 bits private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, deriveRsa1024Successful) {
  TempFile key_file{};
  auto &&path = key_file.create(rsa_1024_private_key_);
  auto &&key = keygen_.importKey(path, "qweqweqwe");
  ASSERT_TRUE(key);
  auto &&val = key.value();
  auto &&derived = keygen_.derivePublicKey(val);
  ASSERT_TRUE(derived);
}

/**
 * @given RSA 2048 bits private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, deriveRsa2048Successful) {
  TempFile key_file{};
  auto &&path = key_file.create(rsa_2048_private_key_);
  auto &&key = keygen_.importKey(path, "qweqweqwe");
  ASSERT_TRUE(key);
  auto &&val = key.value();
  auto &&derived = keygen_.derivePublicKey(val);
  ASSERT_TRUE(derived);
}

/**
 * @given RSA 4096 bits private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, deriveRsa4096Successful) {
  TempFile key_file{};
  auto &&path = key_file.create(rsa_4096_private_key_);
  auto &&key = keygen_.importKey(path, "qweqweqwe");
  ASSERT_TRUE(key);
  auto &&val = key.value();
  auto &&derived = keygen_.derivePublicKey(val);
  ASSERT_TRUE(derived);
}

/**
 * @given ED25519 private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, deriveEd25519uccessful) {
  TempFile key_file{};
  auto &&path = key_file.create(ed25519_private_key_);
  auto &&key = keygen_.importKey(path, "");
  ASSERT_TRUE(key);
  auto &&val = key.value();
  auto &&derived = keygen_.derivePublicKey(val);
  ASSERT_TRUE(derived);
}

/**
 * @given SECP256k1 private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, deriveSecp2561kSuccessful) {
  TempFile key_file{};
  auto &&path = key_file.create(secp256k1_private_key_);
  auto &&key = keygen_.importKey(path, "");
  ASSERT_TRUE(key);
  auto &&val = key.value();
  auto &&derived = keygen_.derivePublicKey(val);
  ASSERT_TRUE(derived);
}
