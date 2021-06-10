addtest(metrics_service_test
    service_test.cpp)

target_link_libraries(metrics_service_test
    Boost::Boost.DI
    metrics)

addtest(metrics_metrics_test
    metrics_test.cpp)

target_link_libraries(metrics_metrics_test
    metrics)
