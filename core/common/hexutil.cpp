#include <algorithm>
#include <boost/algorithm/hex.hpp>

#include "hexutil.hpp"

namespace kagome::common {

  std::string hex(const uint8_t *array, size_t len) noexcept {
    std::string res(len * 2, '\x00');
    boost::algorithm::hex(array, array + len, res.begin());
    return res;
  }

  std::string hex(const std::vector<uint8_t> &bytes) noexcept {
    return hex(bytes.data(), bytes.size());
  }

  std::vector<uint8_t> unhex(const char *array, size_t len) {
    std::vector<uint8_t> blob((len + 1) / 2);
    boost::algorithm::unhex(array, array + len, blob.begin());
    return blob;
  }

  std::vector<uint8_t> unhex(const std::string &hex) {
    return unhex(hex.data(), hex.size());
  }

}  // namespace kagome::common
