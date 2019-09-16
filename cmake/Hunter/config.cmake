hunter_config(
    Boost
    VERSION 1.70.0-p0
)

hunter_config(
    GTest
    VERSION 1.8.0-hunter-p11
    CMAKE_ARGS CMAKE_CXX_FLAGS=-Wno-deprecated-copy
)

hunter_config(
    GMock
    VERSION 1.8.0-hunter-p11
    CMAKE_ARGS CMAKE_CXX_FLAGS=-Wno-deprecated-copy
)
