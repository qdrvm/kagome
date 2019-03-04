/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIBASE_HPP
#define KAGOME_MULTIBASE_HPP

#include <memory>
#include <string>
#include <string_view>

#include "common/buffer.hpp"
#include "common/result.hpp"

namespace libp2p::multi {

  /**
   * Allows to distinguish between different base-encoded binaries
   * See more: https://github.com/multiformats/multibase
   */
  class Multibase {
   private:
    using FactoryResult =
        kagome::expected::Result<std::unique_ptr<Multibase>, std::string>;
    using ByteBuffer = kagome::common::Buffer;

   public:
    /**
     * Encodings, supported by this Multibase
     */
    enum class Encoding { kBase16Lower, kBase16Upper, kBase58, kBase64 };

    /**
     * Create a Multibase instance from the encoded string
     * @param encoded_data - string, which is encoded in one of the supported
     * formats
     * @return Multibase instance, if creation is successful, error otherwise
     */
    static FactoryResult createMultibaseFromEncoded(
        std::string_view encoded_data);

    /**
     * Create a Multibase instance from raw bytes
     * @param decoded_data to be encoded
     * @param base of encoding
     * @return Multibase instance, if creation is successful, error otherwise
     */
    static FactoryResult createMultibaseFromDecoded(
        const ByteBuffer &decoded_data, Encoding base);

    Multibase(const Multibase &other) = default;
    Multibase &operator=(const Multibase &other) = default;
    Multibase(Multibase &&other) = default;
    Multibase &operator=(Multibase &&other) = default;

    /**
     * Get the base of encoding of this Multibase
     * @return the base
     */
    Encoding base() const;

    /**
     * Get the encoded data including the encoding prefix
     * @return the data
     */
    std::string_view encodedData() const;

    /**
     * Get the decoded data
     * @return the data
     */
    const ByteBuffer &decodedData() const;

    bool operator==(const Multibase &rhs) const;
    bool operator!=(const Multibase &rhs) const;

   private:
    Multibase(std::string &&encoded_data, ByteBuffer &&raw_data, Encoding base);

    // encoded data in string format with the encoding prefix
    std::string encoded_data_;

    // decoded data in bytes
    ByteBuffer decoded_data_;

    // base of encoding
    Encoding base_;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIBASE_HPP
