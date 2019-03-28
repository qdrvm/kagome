add_library(primitives
        block.hpp
        block.cpp
        block_header.hpp
        block_header.cpp
        extrinsic.hpp
        extrinsic.cpp
        scale_codec.hpp
        impl/scale_codec_impl.hpp
        impl/scale_codec_impl.cpp
        )

target_link_libraries(primitives
        buffer
        outcome
        scale
        )
