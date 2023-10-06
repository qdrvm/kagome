/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CUSTOM_JSON_WRITER_HPP
#define KAGOME_CUSTOM_JSON_WRITER_HPP

#include <jsonrpc-lean/json.h>
#include <jsonrpc-lean/jsonformatteddata.h>
#include <jsonrpc-lean/util.h>
#include <jsonrpc-lean/value.h>
#include <jsonrpc-lean/writer.h>

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson {
  typedef ::std::size_t SizeType;
}

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace kagome::api {

  /**
   * Custom JsonWriter to format events for pub-sub rpc.
   */
  class JsonWriter final : public jsonrpc::Writer {
   public:
    JsonWriter() : myRequestData(new jsonrpc::JsonFormattedData()) {}

    // Writer
    std::shared_ptr<jsonrpc::FormattedData> GetData() override {
      return std::static_pointer_cast<jsonrpc::FormattedData>(myRequestData);
    }

    void StartDocument() override {
      // Empty
    }

    void EndDocument() override {
      // Empty
    }

    void StartRequest(const std::string &methodName,
                      const jsonrpc::Value &id) override {
      myRequestData->Writer.StartObject();

      myRequestData->Writer.Key(jsonrpc::json::JSONRPC_NAME,
                                sizeof(jsonrpc::json::JSONRPC_NAME) - 1);
      myRequestData->Writer.String(
          jsonrpc::json::JSONRPC_VERSION_2_0,
          sizeof(jsonrpc::json::JSONRPC_VERSION_2_0) - 1);

      myRequestData->Writer.Key(jsonrpc::json::METHOD_NAME,
                                sizeof(jsonrpc::json::METHOD_NAME) - 1);
      myRequestData->Writer.String(methodName.data(), methodName.size(), true);

      WriteId(id);

      myRequestData->Writer.Key(jsonrpc::json::PARAMS_NAME,
                                sizeof(jsonrpc::json::PARAMS_NAME) - 1);
    }

    void EndRequest() override {
      myRequestData->Writer.EndObject();
    }

    void StartParameter() override {
      // Empty
    }

    void EndParameter() override {
      // Empty
    }

    void StartResponse(const jsonrpc::Value &id) override {
      myRequestData->Writer.StartObject();

      myRequestData->Writer.Key(jsonrpc::json::JSONRPC_NAME,
                                sizeof(jsonrpc::json::JSONRPC_NAME) - 1);
      myRequestData->Writer.String(
          jsonrpc::json::JSONRPC_VERSION_2_0,
          sizeof(jsonrpc::json::JSONRPC_VERSION_2_0) - 1);

      WriteId(id);

      myRequestData->Writer.Key(jsonrpc::json::RESULT_NAME,
                                sizeof(jsonrpc::json::RESULT_NAME) - 1);
    }

    void EndResponse() override {
      myRequestData->Writer.EndObject();
    }

    void StartFaultResponse(const jsonrpc::Value &id) override {
      myRequestData->Writer.StartObject();

      myRequestData->Writer.Key(jsonrpc::json::JSONRPC_NAME,
                                sizeof(jsonrpc::json::JSONRPC_NAME) - 1);
      myRequestData->Writer.String(
          jsonrpc::json::JSONRPC_VERSION_2_0,
          sizeof(jsonrpc::json::JSONRPC_VERSION_2_0) - 1);

      WriteId(id);
    }

    void EndFaultResponse() override {
      myRequestData->Writer.EndObject();
    }

    void WriteFault(int32_t code, const std::string &string) override {
      myRequestData->Writer.Key(jsonrpc::json::ERROR_NAME,
                                sizeof(jsonrpc::json::ERROR_NAME) - 1);
      myRequestData->Writer.StartObject();

      myRequestData->Writer.Key(jsonrpc::json::ERROR_CODE_NAME,
                                sizeof(jsonrpc::json::ERROR_CODE_NAME) - 1);
      myRequestData->Writer.Int(code);

      myRequestData->Writer.Key(jsonrpc::json::ERROR_MESSAGE_NAME,
                                sizeof(jsonrpc::json::ERROR_MESSAGE_NAME) - 1);
      myRequestData->Writer.String(string.data(), string.size(), true);

      myRequestData->Writer.EndObject();
    }

    void StartArray() override {
      myRequestData->Writer.StartArray();
    }

    void EndArray() override {
      myRequestData->Writer.EndArray();
    }

    void StartStruct() override {
      myRequestData->Writer.StartObject();
    }

    void EndStruct() override {
      myRequestData->Writer.EndObject();
    }

    void StartStructElement(const std::string &name) override {
      myRequestData->Writer.Key(name.data(), name.size(), true);
    }

    void EndStructElement() override {
      // Empty
    }

    void WriteBinary(const char *data, size_t size) override {
      myRequestData->Writer.String(data, size, true);
    }

    void WriteNull() override {
      myRequestData->Writer.Null();
    }

    void Write(bool value) override {
      myRequestData->Writer.Bool(value);
    }

    void Write(double value) override {
      myRequestData->Writer.Double(value);
    }

    void Write(int32_t value) override {
      myRequestData->Writer.Int(value);
    }

    void Write(int64_t value) override {
      myRequestData->Writer.Int64(value);
    }

    void Write(const std::string &value) override {
      myRequestData->Writer.String(value.data(), value.size(), true);
    }

    void Write(const tm &value) override {
      Write(jsonrpc::util::FormatIso8601DateTime(value));
    }

   private:
    void WriteId(const jsonrpc::Value &id) {
      if (id.IsString() || id.IsInteger32() || id.IsInteger64() || id.IsNil()) {
        myRequestData->Writer.Key(jsonrpc::json::ID_NAME,
                                  sizeof(jsonrpc::json::ID_NAME) - 1);
        if (id.IsString()) {
          myRequestData->Writer.String(
              id.AsString().data(), id.AsString().size(), true);
        } else if (id.IsInteger32()) {
          myRequestData->Writer.Int(id.AsInteger32());
        } else if (id.IsInteger64()) {
          myRequestData->Writer.Int64(id.AsInteger64());
        } else {
          myRequestData->Writer.Null();
        }
      }
    }

    std::shared_ptr<jsonrpc::JsonFormattedData> myRequestData;
  };

}  // namespace kagome::api

#endif  // KAGOME_CUSTOM_JSON_WRITER_HPP
