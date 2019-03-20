add_library(primitives
        block.hpp
        block.cpp
        block_header.hpp
        block_header.cpp
        extrinsic.hpp
        extrinsic.cpp
        )

target_link_libraries(primitives
        buffer
        )
