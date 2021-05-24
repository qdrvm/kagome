#include "registry.hpp"
#include "handler.hpp"

namespace kagome::metrics {

  void Registry::setHandler(Handler *handler) {
    handler->registerCollectable(this);
  }

}  // namespace kagome::metrics
