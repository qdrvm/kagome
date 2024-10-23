/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wabt/version.hpp"

#include "runtime/wabt/util.hpp"

namespace kagome::runtime {
  outcome::result<std::optional<primitives::Version>> readEmbeddedVersion(
      BufferView wasm) {
    OUTCOME_TRY(module, wabtDecode(wasm, {}));
    auto custom_section_contents = [&](std::string_view name) {
      auto it = std::ranges::find_if(
          module.customs,
          [&](const wabt::Custom &section) { return section.name == name; });
      return it != module.customs.end()
               ? std::make_optional(BufferView{it->data})
               : std::nullopt;
    };
    auto version_section = custom_section_contents("runtime_version");
    if (not version_section) {
      return std::nullopt;
    }
    std::optional<primitives::ApisVec> apis;
    std::optional<uint32_t> core_version;
    if (auto apis_section = custom_section_contents("runtime_apis")) {
      apis.emplace();
      scale::ScaleDecoderStream s{*apis_section};
      while (s.hasMore(1)) {
        OUTCOME_TRY(scale::decode(s, apis->emplace_back()));
      }
      core_version = primitives::detail::coreVersionFromApis(*apis);
    }
    scale::ScaleDecoderStream s{*version_section};
    OUTCOME_TRY(version, primitives::Version::decode(s, core_version));
    if (apis) {
      version.apis = std::move(*apis);
    }
    return version;
  }
}  // namespace kagome::runtime
