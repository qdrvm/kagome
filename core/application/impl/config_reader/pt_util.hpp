/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_UTIL_HPP
#define KAGOME_APPLICATION_UTIL_HPP

#include <boost/property_tree/ptree.hpp>
#include <outcome/outcome.hpp>

#include "application/impl/kagome_config.hpp"

namespace kagome::application {

  /**
   * Initialise kagome config parameters from a boost property tree
   */
  outcome::result<KagomeConfig> initConfigFromPropertyTree(
      const boost::property_tree::ptree &tree);

  /**
   * Update kagome config parameters from ones present in the provided boost
   * property tree
   */
  outcome::result<void> updateConfigFromPropertyTree(
      KagomeConfig &config, const boost::property_tree::ptree &tree);

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_UTIL_HPP
