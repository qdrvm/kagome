
add_library(payment_api_service
    payment_jrpc_processor.cpp
    impl/payment_api_impl.cpp
    )
target_link_libraries(payment_api_service
    api_service

    )
