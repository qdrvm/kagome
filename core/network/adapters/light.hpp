/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/bytestr.hpp"
#include "network/adapters/protobuf.hpp"
#include "network/protobuf/light.v1.pb.h"
#include "primitives/common.hpp"

namespace kagome::network {
  struct LightProtocolRequest {
    struct Read {
      std::optional<common::Buffer> child;
      std::vector<common::Buffer> keys;
    };
    struct Call {
      std::string method;
      common::Buffer args;
    };

    primitives::BlockHash block;
    boost::variant<Read, Call> op;
  };

  struct LightProtocolResponse {
    std::vector<common::Buffer> proof;
    bool call = false;
  };

  template <>
  struct ProtobufMessageAdapter<LightProtocolRequest> {
    static size_t size(const LightProtocolRequest &t) {
      return 0;
    }

    static std::vector<uint8_t>::iterator write(
        const LightProtocolRequest &t,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator loaded) {
      ::protobuf_generated::api::v1::light::Request msg;
      if (auto call = boost::get<LightProtocolRequest::Call>(&t.op)) {
        auto &pb = *msg.mutable_remote_call_request();
        pb.set_block(std::string{byte2str(t.block)});
        pb.set_method(call->method);
        pb.set_data(std::string{byte2str(call->args)});
      } else {
        auto &read = boost::get<LightProtocolRequest::Read>(t.op);
        if (read.child) {
          auto &pb = *msg.mutable_remote_read_child_request();
          pb.set_block(std::string{byte2str(t.block)});
          pb.set_storage_key(std::string{byte2str(*read.child)});
          for (auto &key : read.keys) {
            pb.add_keys(std::string{byte2str(key)});
          }
        } else {
          auto &pb = *msg.mutable_remote_read_request();
          pb.set_block(std::string{byte2str(t.block)});
          for (auto &key : read.keys) {
            pb.add_keys(std::string{byte2str(key)});
          }
        }
      }
      return appendToVec(msg, out, loaded);
    }

    static outcome::result<std::vector<uint8_t>::const_iterator> read(
        LightProtocolRequest &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      const auto remains = src.size() - std::distance(src.begin(), from);
      BOOST_ASSERT(remains >= size(out));

      ::protobuf_generated::api::v1::light::Request msg;
      if (not msg.ParseFromArray(from.base(), remains)) {
        return AdaptersError::PARSE_FAILED;
      }

      auto get_block = [&](auto &pb) {
        auto r = primitives::BlockHash::fromSpan(str2byte(pb.block()));
        if (r) {
          out.block = r.value();
        }
        return r;
      };
      if (msg.has_remote_call_request()) {
        auto &pb = msg.remote_call_request();
        OUTCOME_TRY(get_block(pb));
        out.op = LightProtocolRequest::Call{
            pb.method(), common::Buffer{str2byte(pb.data())}};
      } else {
        LightProtocolRequest::Read read;
        auto get_keys = [&](auto &pb) {
          for (auto &key : pb.keys()) {
            read.keys.emplace_back(str2byte(key));
          }
        };
        if (msg.has_remote_read_child_request()) {
          auto &pb = msg.remote_read_child_request();
          OUTCOME_TRY(get_block(pb));
          read.child = common::Buffer{str2byte(pb.storage_key())};
          get_keys(pb);
        } else {
          auto &pb = msg.remote_read_request();
          OUTCOME_TRY(get_block(pb));
          get_keys(pb);
        }
        out.op = std::move(read);
      }

      std::advance(from, msg.ByteSizeLong());
      return from;
    }
  };

  template <>
  struct ProtobufMessageAdapter<LightProtocolResponse> {
    static size_t size(const LightProtocolResponse &t) {
      return 0;
    }

    static std::vector<uint8_t>::iterator write(
        const LightProtocolResponse &t,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator loaded) {
      ::protobuf_generated::api::v1::light::Response msg;
      auto &proof = t.call
                      ? *msg.mutable_remote_call_response()->mutable_proof()
                      : *msg.mutable_remote_read_response()->mutable_proof();
      proof = byte2str(scale::encode(t.proof).value());
      return appendToVec(msg, out, loaded);
    }

    static outcome::result<std::vector<uint8_t>::const_iterator> read(
        LightProtocolResponse &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      const auto remains = src.size() - std::distance(src.begin(), from);
      BOOST_ASSERT(remains >= size(out));

      ::protobuf_generated::api::v1::light::Response msg;
      if (not msg.ParseFromArray(from.base(), remains)) {
        return AdaptersError::PARSE_FAILED;
      }

      out.call = msg.has_remote_call_response();
      OUTCOME_TRY(proof,
                  scale::decode<std::vector<common::Buffer>>(
                      str2byte(out.call ? msg.remote_call_response().proof()
                                        : msg.remote_read_response().proof())));
      out.proof = std::move(proof);

      std::advance(from, msg.ByteSizeLong());
      return from;
    }
  };

}  // namespace kagome::network
