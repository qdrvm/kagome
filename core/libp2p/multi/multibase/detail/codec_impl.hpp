/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIBASE_CODEC_IMPL_HPP
#define KAGOME_MULTIBASE_CODEC_IMPL_HPP

#include <map>

#include "libp2p/multi/multibase/codec.hpp"

namespace libp2p::multi {

  /**
   * Implementation of the codec
   */
  class Codec::Impl {
   public:
    Multibase::Encoding base();

    bool is_valid(const cstring_span &input, bool include_encoding = true);

    /** Encode to the input, optionally including the encoding type in the
     * output
     */
    std::string encode(const cstring_span &input, bool include_encoding = true);

    /** Encode the input, writing the result to the user-supplied buffer,
     * optionally including the encoding type in the output
     * @return Number of bytes written */
    std::size_t encode(const cstring_span &input,
                       string_span &output,
                       bool include_encoding = true);

    std::size_t encoded_size(const cstring_span &input,
                             bool include_encoding = true);

    std::string decode(const cstring_span &input);

    std::size_t decode(const cstring_span &input, string_span &output);

    std::size_t decoded_size(const cstring_span &input);

    /** Registry of codec implementations */
    class registry {
     public:
      registry() = default;
      using key_type = encoding;
      using mapped_type = std::shared_ptr<codec::impl>;
      using value_type = std::pair<key_type, mapped_type>;
      mapped_type &operator[](const key_type &key);

     private:
      using data_type = std::map<key_type, mapped_type>;
      static data_type &data();
    };

   protected:
    class impl_tag {};
    virtual bool is_valid(const cstring_span &input, impl_tag) = 0;
    virtual std::size_t encode(const cstring_span &input,
                               string_span &output,
                               impl_tag) = 0;
    virtual std::size_t decode(const cstring_span &input,
                               string_span &output,
                               impl_tag) = 0;
    virtual encoding get_encoding() = 0;
    virtual std::size_t get_encoded_size(const cstring_span &input) = 0;
    virtual std::size_t get_decoded_size(const cstring_span &input) = 0;

   private:
    std::size_t encoding_size(bool include_encoding);
    std::size_t write_encoding(string_span &output, bool include_encoding);
  };

}  // namespace libp2p::multi

#endif  // KAGOME_MULTIBASE_CODEC_IMPL_HPP
