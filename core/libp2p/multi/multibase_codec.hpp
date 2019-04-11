/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIBASE_HPP
#define KAGOME_MULTIBASE_HPP

#include <string>
#include <string_view>
#include <utility>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace libp2p::multi {
  /**
   * Allows to distinguish between different base-encoded binaries
   * See more: https://github.com/multiformats/multibase
   */
  class MultibaseCodec {
    using ByteBuffer = kagome::common::Buffer;
    using FactoryResult = outcome::result <ByteBuffer>;

   public:
    /**
     * Encodings, supported by this Multibase
     */
    enum class Encoding : char {
      kBase16Lower = 'f',
      kBase16Upper = 'F',
      kBase58 = 'Z',
      kBase64 = 'm'
    };

    /**
     * Encode the incoming bytes
     * @param bytes to be encoded
     * @param encoding - base of the desired encoding
     * @return encoded string WITH an encoding prefix
     */
    virtual std::string encode(const ByteBuffer &bytes,
                               Encoding encoding) const = 0;

    /**
     * Decode the incoming string
     * @param string to be decoded
     * @return bytes, if decoding was successful, error otherwise
     */
    virtual FactoryResult decode(std::string_view string) const = 0;

    virtual ~MultibaseCodec() = 0;
  };

  inline MultibaseCodec::~MultibaseCodec() = default;
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIBASE_HPP
