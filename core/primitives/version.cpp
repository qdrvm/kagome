/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/version.hpp"

namespace kagome::primitives {

  Version::Version(std::string spec_name, std::string impl_name,
                   uint32_t authoring_version, uint32_t impl_version,
                   const std::vector &apis)
      : spec_name_(spec_name),
        impl_name_(impl_name),
        authoring_version_(authoring_version),
        impl_version_(impl_version),
        apis_(apis) {}

  const std::string &Version::spec_name() {
    return spec_name_;
  }

  const std::string &Version::impl_name() {
    return impl_name_;
  }

  uint32_t Version::authoring_version() {
    return authoring_version_;
  }

  uint32_t Version::impl_version() {
    return impl_version_;
  }

  const std::vector<std::string> &Version::apis() {
    return apis_;
  }

}  // namespace kagome::primitives
