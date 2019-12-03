/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_EXAMPLE_CONFIG_HPP
#define KAGOME_APPLICATION_EXAMPLE_CONFIG_HPP

#include "application/impl/kagome_config.hpp"

namespace kagome::application {

  const kagome::application::KagomeConfig &getExampleConfig();
  std::stringstream readJSONConfig();
}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_EXAMPLE_CONFIG_HPP
