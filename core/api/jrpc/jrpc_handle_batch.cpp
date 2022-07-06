/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/jrpc/jrpc_handle_batch.hpp"

#include <jsonrpc-lean/server.h>

namespace kagome::api {
  /**
   * Parses jsonrpc batch request.
   * Callback is invoked for each request.
   */
  template <typename Cb>
  struct Parser {
    Cb cb;

    rapidjson::MemoryStream stream{nullptr, 0};
    size_t level = 0;
    size_t begin = 0;

    bool parse(std::string_view request) {
      stream = {request.data(), request.size()};
      rapidjson::Reader reader;
      return reader.Parse(stream, *this);
    }

    bool scalar() const {
      return level > 1;
    }
    bool Null() {
      return scalar();
    }
    bool Bool(bool) {
      return scalar();
    }
    bool Int(int) {
      return scalar();
    }
    bool Uint(unsigned) {
      return scalar();
    }
    bool Int64(int64_t) {
      return scalar();
    }
    bool Uint64(uint64_t) {
      return scalar();
    }
    bool Double(double) {
      return scalar();
    }
    bool RawNumber(const char *, size_t, bool) {
      return scalar();
    }
    bool String(const char *, size_t, bool) {
      return scalar();
    }
    bool Key(const char *, size_t, bool) {
      return scalar();
    }
    bool StartArray() {
      if (level == 1) {
        return false;
      }
      ++level;
      return true;
    }
    bool EndArray(size_t) {
      --level;
      return true;
    }
    bool StartObject() {
      if (level == 0) {
        return false;
      }
      if (level == 1) {
        begin = stream.Tell() - 1;
      }
      ++level;
      return true;
    }
    bool EndObject(size_t) {
      --level;
      if (level == 1) {
        const size_t end = stream.Tell();
        std::string_view request(stream.begin_ + begin, end - begin);
        cb(request);
      }
      return true;
    }
  };

  JrpcHandleBatch::JrpcHandleBatch(jsonrpc::Server &handler,
                                   std::string_view request) {
    std::string request_string;
    if (!request.empty() && request[0] == '[') {
      const auto cb = [&](std::string_view request) {
        request_string = request;
        auto formatted = handler.HandleRequest(request_string);
        if (formatted->GetSize() == 0) {
          return;
        }
        if (batch_.empty()) {
          batch_.push_back('[');
        } else {
          batch_.push_back(',');
        }
        batch_.append(formatted->GetData(), formatted->GetSize());
      };
      Parser<decltype(cb) &> parser{cb};
      if (parser.parse(request)) {
        if (!batch_.empty()) {
          batch_.push_back(']');
        }
        return;
      }
    }
    request_string = request;
    formatted_ = handler.HandleRequest(request_string);
  }

  std::string_view JrpcHandleBatch::response() const {
    if (formatted_ == nullptr) {
      return batch_;
    }
    return {formatted_->GetData(), formatted_->GetSize()};
  }
}  // namespace kagome::api
