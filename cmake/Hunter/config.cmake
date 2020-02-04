hunter_config(
    Boost
    VERSION 1.70.0-p0
    CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=ON
)

hunter_config(
    GTest
    VERSION 1.8.0-hunter-p11
    CMAKE_ARGS "CMAKE_CXX_FLAGS=-Wno-deprecated-copy"
)

hunter_config(sr25519
    URL https://github.com/Warchant/sr25519-crust/archive/2947abb8367d57cd712e8bc80687d224ccd86ccf.zip
    SHA1 2b0f06efba6846fd66f8de397179b1b955af8da6
)

hunter_config(
    spdlog
    URL https://github.com/gabime/spdlog/archive/v1.4.2.zip
    SHA1 4b10e9aa17f7d568e24f464b48358ab46cb6f39c
)

hunter_config(
    tsl_hat_trie
    URL https://github.com/masterjedy/hat-trie/archive/343e0dac54fc8491065e8a059a02db9a2b1248ab.zip
    SHA1 7b0051e9388d629f382752dd6a12aa8918cdc022
)

hunter_config(
    Boost.DI
    URL https://github.com/masterjedy/di/archive/c5287ee710ad90f5286d0cc2b9e49b72d89267a6.zip
    SHA1 802b64a6242be45771f3d4c86257eac0a3c7b289
    # disable building examples and tests, disable testing
    CMAKE_ARGS BOOST_DI_OPT_BUILD_TESTS=OFF BOOST_DI_OPT_BUILD_EXAMPLES=OFF
)

hunter_config(libp2p
    URL https://github.com/soramitsu/libp2p/archive/a93e6157b3b8d03b3534a4217e557d52b874d8ec.zip
    SHA1 49b21c3f5f27dd7a26533e719559fc51320194a7
    CMAKE_ARGS TESTING=OFF
    )
