/**
* Copyright Quadrivium LLC
* All Rights Reserved
* SPDX-License-Identifier: Apache-2.0
 */

#include <openssl/crypto.h>>

namespace kagome::crypto {

    void cleanse(void* ptr, size_t size) {
      OPENSSL_cleanse(ptr, size);
    }


}