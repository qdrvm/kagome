/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READWRITECLOSER_HPP
#define KAGOME_READWRITECLOSER_HPP

#include "libp2p/basic/closeable.hpp"
#include "libp2p/basic/reader.hpp"
#include "libp2p/basic/writer.hpp"

namespace libp2p::basic {

  struct ReadWriteCloser : public Reader, public Writer, public Closeable {
    ~ReadWriteCloser() override = default;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_READWRITECLOSER_HPP
