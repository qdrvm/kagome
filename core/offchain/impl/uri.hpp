/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_URI
#define KAGOME_OFFCHAIN_URI

#include <optional>
#include <string>
#include <string_view>

namespace kagome::offchain {
  struct Uri final {
   private:
    std::string uri_;
    std::optional<std::string_view> error_ = "Is not initialized";

    ptrdiff_t schema_offset_ = 0;
    ptrdiff_t host_offset_ = 0;
    ptrdiff_t port_offset_ = 0;
    ptrdiff_t path_offset_ = 0;
    ptrdiff_t query_offset_ = 0;
    ptrdiff_t fragment_offset_ = 0;
    size_t schema_size_ = 0;
    size_t host_size_ = 0;
    size_t port_size_ = 0;
    size_t path_size_ = 0;
    size_t query_size_ = 0;
    size_t fragment_size_ = 0;

    void reset() {
      uri_.clear();
      error_.emplace("Is not initialized");
      schema_offset_ = 0;
      schema_offset_ = 0;
      host_offset_ = 0;
      port_offset_ = 0;
      path_offset_ = 0;
      query_offset_ = 0;
      fragment_offset_ = 0;
      schema_size_ = 0;
      host_size_ = 0;
      port_size_ = 0;
      path_size_ = 0;
      query_size_ = 0;
      fragment_size_ = 0;
    }

   public:
    Uri() = default;
    Uri(const Uri &other) = default;
    Uri(Uri &&other) noexcept;

    Uri &operator=(const Uri &other) = default;
    Uri &operator=(Uri &&other) noexcept;

    inline std::string_view schema() const {
      return {uri_.data() + schema_offset_, schema_size_};
    };
    inline std::string_view host() const {
      return {uri_.data() + host_offset_, host_size_};
    };
    inline std::string_view port() const {
      return {uri_.data() + port_offset_, port_size_};
    };
    inline std::string_view path() const {
      return {uri_.data() + path_offset_, path_size_};
    };
    inline std::string_view query() const {
      return {uri_.data() + query_offset_, query_size_};
    };
    inline std::string_view fragment() const {
      return {uri_.data() + fragment_offset_, fragment_size_};
    };

    const std::string &toString() const {
      return uri_;
    }

    const std::optional<std::string_view> error() const {
      return error_;
    }

    static Uri Parse(std::string_view uri);
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_URI
