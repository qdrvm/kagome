addtest(keccak_test
    keccak_test.cpp
    )
target_link_libraries(keccak_test
    keccak
    hexutil
    )
