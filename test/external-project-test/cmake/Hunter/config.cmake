
# include kagome hunter config, as it is not applied otherwise which leads to
# kagome build failure
include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/Hunter/config.cmake)

hunter_config(kagome GIT_SELF)
