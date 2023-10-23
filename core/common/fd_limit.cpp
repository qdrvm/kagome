/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/fd_limit.hpp"

#include <sys/resource.h>
#include <algorithm>
#include <boost/iterator/counting_iterator.hpp>

#include "log/logger.hpp"

namespace kagome::common {
  namespace {
    inline auto log() {
      return log::createLogger("FdLimit", log::defaultGroupName);
    }

    bool getFdLimit(rlimit &r) {
      if (getrlimit(RLIMIT_NOFILE, &r) != 0) {
        SL_WARN(log(),
                "Error: getrlimit(RLIMIT_NOFILE) errno={} {}",
                errno,
                strerror(errno));
        return false;
      }
      return true;
    }

    bool setFdLimit(const rlimit &r) {
      return setrlimit(RLIMIT_NOFILE, &r) == 0;
    }
  }  // namespace

  std::optional<size_t> getFdLimit() {
    rlimit r{};
    if (!getFdLimit(r)) {
      return std::nullopt;
    }
    return r.rlim_cur;
  }

  void setFdLimit(size_t limit) {
    rlimit r{};
    if (!getFdLimit(r)) {
      return;
    }
    if (r.rlim_max == RLIM_INFINITY) {
      SL_VERBOSE(log(), "current={} max=unlimited", r.rlim_cur);
    } else {
      SL_VERBOSE(log(), "current={} max={}", r.rlim_cur, r.rlim_max);
    }
    const rlim_t current = r.rlim_cur;
    if (limit == current) {
      return;
    }
    r.rlim_cur = limit;
    if (limit < current) {
      SL_WARN(log(), "requested limit is lower than system allowed limit");
      setFdLimit(r);
    } else if (!setFdLimit(r)) {
      std::ignore = std::upper_bound(boost::counting_iterator{current},
                                     boost::counting_iterator{rlim_t{limit}},
                                     nullptr,
                                     [&](std::nullptr_t, rlim_t current) {
                                       r.rlim_cur = current;
                                       return !setFdLimit(r);
                                     });
    }
    if (!getFdLimit(r)) {
      return;
    }
    if (r.rlim_cur != current) {
      SL_VERBOSE(log(), "changed current={}", r.rlim_cur);
    }
  }
}  // namespace kagome::common
