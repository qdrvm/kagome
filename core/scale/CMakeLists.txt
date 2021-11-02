
add_library(libp2p_types
    libp2p_types.cpp
    )

target_link_libraries(libp2p_types
    scale::scale
    )