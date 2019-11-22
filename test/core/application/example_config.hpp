/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_EXAMPLE_CONFIG_HPP
#define KAGOME_APPLICATION_EXAMPLE_CONFIG_HPP

#include "application/impl/kagome_config.hpp"

using kagome::application::KagomeConfig;

namespace test::application {

  const KagomeConfig &getExampleConfig();
  std::stringstream readJSONConfig();
}  // namespace test

#endif  // KAGOME_APPLICATION_EXAMPLE_CONFIG_HPP
