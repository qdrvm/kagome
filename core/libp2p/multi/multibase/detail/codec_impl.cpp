/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase/detail/codec_impl.hpp"
#include "libp2p/multi/multibase/basic_codec.hpp"
#include "libp2p/multi/multibase/detail/codec_impl.hpp"

namespace libp2p::multi {

  std::size_t codec::impl::encoding_size(bool include_encoding) {
    return include_encoding ? sizeof(encoding_t) : 0;
  }

  std::size_t codec::impl::encoded_size(const cstring_span &input,
                                        bool include_encoding) {
    return get_encoded_size(input) + encoding_size(include_encoding);
  }

  std::size_t codec::impl::write_encoding(string_span &output,
                                          bool include_encoding) {
    if (!include_encoding)
      return 0;
    auto bytes_written = encoding_size(include_encoding);
    if (bytes_written > output.size())
      throw std::out_of_range("Output buffer too small");
    auto data = get_encoding();
    std::copy(reinterpret_cast<char *>(&data),
              reinterpret_cast<char *>(&data + sizeof(data)),
              const_cast<char *>(output.data()));
    return bytes_written;
  }

  std::string codec::impl::encode(const cstring_span &input,
                                  bool include_encoding) {
    auto result = std::string(encoded_size(input, include_encoding), 0);
    auto output = string_span(result);
    auto size = encode(input, output, include_encoding);
    size += include_encoding ? 1 : 0;
    result.resize(size);
    return result;
  }

  std::size_t codec::impl::encode(const cstring_span &input,
                                  string_span &output,
                                  bool include_encoding) {
    auto basic_size = get_encoded_size(input);
    auto size = basic_size + encoding_size(include_encoding);
    if (output.size() < size)
      throw std::out_of_range("Output buffer too small");
    auto begin = &output[0];
    auto view = string_span(begin + write_encoding(output, include_encoding),
                            basic_size);
    return encode(input, view, impl_tag{});
  }

  std::size_t codec::impl::decoded_size(const cstring_span &input) {
    return get_decoded_size(input);
  }

  std::size_t codec::impl::decode(const cstring_span &input,
                                  string_span &output) {
    if (get_decoded_size(input) > output.size())
      throw std::out_of_range("Output buffer too small");
    return decode(input, output, impl_tag{});
  }

  std::string codec::impl::decode(const cstring_span &input) {
    auto size = decoded_size(input);
    auto output = std::string(size, 0);
    auto view = string_span(output);
    output.resize(decode(input, view));
    return output;
  }

  encoding codec::impl::base() {
    return get_encoding();
  }

  bool codec::impl::is_valid(const cstring_span &input, bool include_encoding) {
    auto valid = true;
    if (include_encoding) {
      if (input.empty()) {
        valid = false;
      } else {
        valid = input[0] == static_cast<char>(base());
      }
    }
    if (valid) {
      valid = is_valid(input, impl_tag{});
    }
    return valid;
  }

  codec::impl::registry::data_type &codec::impl::registry::data() {
    static data_type data_ =
        data_type{{encoding::base_16, std::make_unique<base_16>()},
                  {encoding::base_58_btc, std::make_unique<base_58_btc>()}};
    return data_;
  }

  codec::impl::registry::mapped_type &codec::impl::registry::operator[](
      const codec::impl::registry::key_type &key) {
    return data()[key];
  }

}  // namespace libp2p::multi
