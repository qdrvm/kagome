/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISTREAM_HPP
#define KAGOME_MULTISTREAM_HPP

#include <gsl/span>
#include "common/buffer.hpp"
#include "common/result.hpp"

namespace libp2p::multi {
  /**
   * Format of stream identifier used in Libp2p
   * @see https://github.com/multiformats/multistream
   */
  class Multistream {
   public:
    /**
     * A protocol used in the stream is represented as a UNIX URI instead of
     * just its name, since it's much more descriptive
     */
    using Path = std::string;

    /**
     * Creates an empty multistream
     */
    Multistream() noexcept = default;

    /**
     * Creates a Multistream object from
     * a URI, which contains info about the protocol of the stream,
     * @example /http/w3id.org/ipfs/1.1.0
     * and a binary buffer with the stream content.
     * @returns a Result object that contains either the Multistream, or an
     * error, if any occured
     */
    static auto create(Path codecPath, const kagome::common::Buffer &data)
        -> kagome::expected::Result<Multistream, std::string>;

    /**
     * Creates a Multistream object from
     * a buffer with bytes representing a Multistream.
     * <varint-length>'/'<codec-path>'\n'<data>
     * @returns a Result object that contains either the Multistream, or an
     * error, if any occured
     */
    static auto create(const kagome::common::Buffer &bytes)
        -> kagome::expected::Result<Multistream, std::string>;

    /**
     * Adds a prefix to the multistream protocol path ('/path -> /prefix/path')
     * The prefix must not contain line breaks, forward slashes or be empty
     * @return this multistream with prefix added to the beginning of the
     * protocol path or error, if the prefix was invalid
     */
    auto addPrefix(std::string_view prefix)
        -> kagome::expected::Result<std::reference_wrapper<Multistream>,
                                    std::string>;

    /**
     * @return this multistream with the prefix removed from the protocol path
     * or Error if the prefix was not present in the path or removal leaves the
     * path empty
     */
    auto removePrefix(std::string_view prefix)
        -> kagome::expected::Result<std::reference_wrapper<Multistream>,
                                    std::string>;

    /**
     * @return the URI with information about the protocol that is used in the
     * stream
     */
    auto getProtocolPath() const -> const Path &;

    /**
     * @return the content of the stream
     */
    auto getEncodedData() const -> gsl::span<const uint8_t>;

    /**
     * @return the buffer that contains the whole multistream
     */
    auto getBuffer() const -> const kagome::common::Buffer &;

    bool operator==(const Multistream &other) const;

   private:
    Multistream(Path codecPath, const kagome::common::Buffer &data);

    void initBuffer(std::string_view protocol_path,
                    gsl::span<const uint8_t> data);
    void initData();

    Path protocol_path_;
    kagome::common::Buffer multistream_buffer_;
    gsl::span<const uint8_t> data_;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTISTREAM_HPP
