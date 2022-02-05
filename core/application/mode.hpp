/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_MODE
#define KAGOME_APPLICATION_MODE

namespace kagome::application {

  class Mode {
   public:
    virtual ~Mode() = default;

    virtual int run() const = 0;
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_MODE
