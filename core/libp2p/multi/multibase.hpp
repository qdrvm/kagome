/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIBASE_HPP
#define KAGOME_MULTIBASE_HPP

#include <memory>
#include <string>
#include <string_view>

#include "common/result.hpp"
#include "libp2p/multi/multibase/basic_codec.hpp"

namespace libp2p::multi {

  /**
   * Allows to distinguish between different base-encoded binaries
   * See more: https://github.com/multiformats/multibase
   * Core implementation is taken from https://github.com/cpp-ipfs/cpp-multibase
   */
  class Multibase {
   private:
    using FactoryResult =
        kagome::expected::Result<std::unique_ptr<Multibase>, std::string>;

   public:
    /**
     * Encodings, supported by this Multibase
     */
    enum class Encoding : unsigned char {
      base_16 = 'f',
      base_16_upper = 'F',
      base_32 = 'b',
      base_32_upper = 'B',
      base_58_btc = 'Z',
      base_64 = 'm'
    };

    /**
     * Create a Multibase instance from encoded string
     * @param encoded_data - the data
     * @return Multibase instance, if creation is successful, error otherwise
     */
    static FactoryResult createMultibaseFromEncoded(
        std::string_view encoded_data);

    /**
     * Create a Multibase instance from raw string
     * @param data to be encoded
     * @param base of encode
     * @return Multibase instance, if creation is successful, error otherwise
     */
    static FactoryResult createMultibaseFromRaw(std::string_view raw_data,
                                                Encoding base);

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
     * Get the encoded data
     * @return the data
     */
    std::string_view encodedData() const;

    /**
     * Get the raw data
     * @return the data
     */
    std::string_view rawData() const;

    bool operator==(const Multibase &rhs) const;
    bool operator!=(const Multibase &rhs) const;

   private:
    Multibase(std::string &&encoded_data,
              std::string &&raw_data,
              Encoding base);

    // encoded data without base character
    std::string encoded_data_;
    // decoded data
    std::string raw_data_;
    // base of encode
    Encoding base_;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIBASE_HPP
