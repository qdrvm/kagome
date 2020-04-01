
add_library(trie_db_overlay
    impl/trie_db_overlay_impl.cpp
    )
target_link_libraries(trie_db_overlay
    buffer
    )
