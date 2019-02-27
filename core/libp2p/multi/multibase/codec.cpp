/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase/codec.hpp"
#include "libp2p/multi/multibase/detail/codec_impl.hpp"

namespace libp2p::multi {

  Codec::Codec(Multibase::Encoding base)
      : impl_(std::make_unique<Impl>(Codec::Impl::registry()[base].get())) {
    if (!impl_) {
      auto msg = std::string("No codec implementation for encoding ");
      msg.push_back(static_cast<char>(base));
      throw std::out_of_range(msg);
    }
  }

  std::string Codec::encode(std::string_view input, bool include_encoding) {
    return impl_->encode(input, include_encoding);
  }

  std::string Codec::decode(std::string_view input) {
    return impl_->decode(input);
  }

  Multibase::Encoding Codec::base() const {
    return impl_->base();
  }

}  // namespace libp2p::multi
