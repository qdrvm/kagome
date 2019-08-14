add_subdirectory(host)

addtest(all_muxers_acceptance_test
    muxer.cpp
    )
target_link_libraries(all_muxers_acceptance_test
    libp2p_yamux
    multiaddress
    libp2p_plaintext
    libp2p_tcp
    libp2p_default_network
    libp2p_basic_host
    testutil
    marshaller
    libp2p_identity_manager
    )
