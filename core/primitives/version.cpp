/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/version.hpp"

namespace kagome::primitives {

  Version::Version(std::string spec_name, std::string impl_name,
                   uint32_t authoring_version, uint32_t impl_version,
                   ApisVec apis)
      : spec_name_(std::move(spec_name)),
        impl_name_(std::move(impl_name)),
        authoring_version_(authoring_version),
        impl_version_(impl_version),
        apis_(std::move(apis)) {}

  const std::string &Version::specName() const {
    return spec_name_;
  }

  const std::string &Version::implName() const {
    return impl_name_;
  }

  uint32_t Version::authoringVersion() const {
    return authoring_version_;
  }

  uint32_t Version::implVersion() const {
    return impl_version_;
  }

  const ApisVec &Version::apis() const {
    return apis_;
  }

}  // namespace kagome::primitives
