/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pvm/types.hpp"
#include <sys/syscall.h>
#include <errno.h>

namespace kagome::pvm::native::linux {
    using Fd = decltype(syscall());

    inline Result<void> check_syscall(Fd result) {
        auto unprivileged_userfaultfd = []() {
            std::ifstream file("/proc/sys/vm/unprivileged_userfaultfd", std::ios::binary);
            if (file) {
                char buffer[2];
                if (file.read(buffer, 2)) {
                    return buffer[0] == '0' && buffer[1] == '\n';
                }
            }
            return false;
        };

        if (result >= -4095 && result < 0) {
            if (errno == EPERM && unprivileged_userfaultfd()) {
                return Error::SYS_CALL_NOT_PERMITTED;
            }
            return Error::SYS_CALL_FAILED;
        }
        return outcome::success();
    }

    inline Result<Fd> sys_userfaultfd(unsigned int flags) {
        auto fd = syscall(SYS_userfaultfd, flags);
        OUTCOME_TRY(check_syscall(fd));
        return outcome::success(fd);
    }

}
