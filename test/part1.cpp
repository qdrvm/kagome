#include <gtest/gtest.h>

#include "libp2p/common_objects/multiaddress.hpp"
#include "libp2p/common_objects/multihash.hpp"
#include "libp2p/common_objects/peer.hpp"
#include "libp2p/routing/router.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/transport/transport.hpp"
#include "libp2p/transport/transport_listener.hpp"
#include "libp2p/store/record.hpp"
#include "libp2p/store/record_store.hpp"

TEST(Part, One){
  printf("Part1!");
}
