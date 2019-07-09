addtest(all_muxers_acceptance_test
    muxer.cpp
    )
target_link_libraries(all_muxers_acceptance_test
    libp2p_yamux
    multiaddress
    libp2p_plaintext
    libp2p_tcp
    testutil
    marshaller
    libp2p_identity_manager
    )


#addtest(network_acceptance_test
#    network.cpp
#    )
#target_link_libraries(network_acceptance_test
#    libp2p_network
#    libp2p_tcp
#    libp2p_yamux
#    libp2p_plaintext
#    libp2p_upgrader
#    libp2p_transport_manager
#    libp2p_connection_manager
#    libp2p_router
#    libp2p_inmem_address_repository
#    libp2p_protocol_echo
#    marshaller
#    multiselect
#    libp2p_identity_manager
#    )
