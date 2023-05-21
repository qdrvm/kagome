
add_library(filesystem filesystem.cpp)
target_link_libraries(filesystem Boost::filesystem)
kagome_install(filesystem)
