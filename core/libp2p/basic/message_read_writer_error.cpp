/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/message_read_writer_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::basic, MessageReadWriterError, e) {
  using E = libp2p::basic::MessageReadWriterError;
  switch (e) {
    case E::VARINT_EXPECTED:
      return "varint expected at the beginning of Protobuf message";
  }
  return "unknown error";
}
