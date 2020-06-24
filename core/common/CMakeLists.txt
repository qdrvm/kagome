add_library(hexutil
    hexutil.hpp
    hexutil.cpp
    )
target_link_libraries(hexutil
    Boost::boost
    outcome
    )
kagome_install(hexutil)

add_library(buffer
    buffer.hpp
    buffer.cpp
    )
target_link_libraries(buffer
    hexutil
    )
kagome_install(buffer)

add_library(blob
    blob.hpp
    blob.cpp
    )
target_link_libraries(blob
    hexutil
    )
kagome_install(blob)


add_library(logger
    logger.cpp
    )
target_link_libraries(logger
    spdlog::spdlog
    )
kagome_install(logger)

add_library(mp_utils
    mp_utils.cpp
    mp_utils.hpp
    )
target_link_libraries(mp_utils
    Boost::boost
    )
kagome_install(mp_utils)
