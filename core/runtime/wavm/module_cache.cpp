#include "runtime/wavm/module_cache.hpp"

#include <fstream>
#include <vector>

#include "crypto/hasher.hpp"
#include "filesystem/directories.hpp"

namespace kagome::runtime::wavm {
  ModuleCache::ModuleCache(const application::AppConfiguration &app_config,
                           std::shared_ptr<crypto::Hasher> hasher)
      : app_config_(app_config),
        hasher_{std::move(hasher)},
        logger_{log::createLogger("WAVM Module Cache", "runtime_cache")} {
    BOOST_ASSERT(hasher_ != nullptr);
  }

  std::vector<WAVM::U8> ModuleCache::getCachedObject(
      const WAVM::U8 *wasmBytes,
      WAVM::Uptr numWASMBytes,
      std::function<std::vector<WAVM::U8>()> &&compileThunk) {
    namespace fs = kagome::filesystem;
    auto runtime_hash =
        hasher_->twox_64(gsl::span(wasmBytes, numWASMBytes)).toHex();
    auto filepath = app_config_.cachedRuntimePath(runtime_hash);
    if (!exists(filepath) and !exists(filepath.parent_path())
        and !fs::createDirectoryRecursive(filepath.parent_path())) {
      SL_ERROR(logger_,
               "Failed to create runtimes cache directory {}",
               filepath.parent_path());
    }

    std ::vector<WAVM::U8> module;
    if (auto file = std::ifstream{filepath, std::ios::in | std::ios::binary};
        file.is_open()) {
      auto module_size = file_size(filepath);
      module.resize(module_size);
      file.read(reinterpret_cast<char *>(module.data()), module_size);
      SL_INFO(logger_, "WAVM runtime cache hit: {}", filepath);
    }
    if (module.empty()) {
      module = compileThunk();
      if (auto file = std::ofstream{filepath, std::ios::out | std::ios::binary};
          file.is_open()) {
        file.write(reinterpret_cast<char *>(module.data()), module.size());
        SL_INFO(logger_, "Saved WAVM runtime to cache: {}", filepath);
      } else {
        SL_ERROR(logger_, "Failed to cache WAVM runtime: {}", filepath);
      }
    }

    return module;
  }
}  // namespace kagome::runtime::wavm
