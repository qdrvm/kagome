# Set the default system name and processor
if(NOT DEFINED CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "")
    set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
endif()

if(NOT DEFINED CMAKE_SYSTEM_PROCESSOR OR CMAKE_SYSTEM_PROCESSOR STREQUAL "")
    if(DEFINED CMAKE_HOST_SYSTEM_PROCESSOR AND NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "")
        set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})
    elseif(UNIX)
        execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE _uname_m
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(CMAKE_SYSTEM_PROCESSOR ${_uname_m})
    else()
        set(CMAKE_SYSTEM_PROCESSOR "unknown")
    endif()
endif()

# Задаём HUNTER_ROOT в каталоге ~/.hunter/<system name>-<processor>
set(HUNTER_ROOT "$ENV{HOME}/.hunter/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" CACHE PATH "Hunter root directory")

# specify GITHUB_HUNTER_TOKEN and GITHUB_HUNTER_USERNAME to automatically upload binary cache to github.com/qdrvm/hunter-binary-cache
# https://hunter.readthedocs.io/en/latest/user-guides/hunter-user/github-cache-server.html
string(COMPARE EQUAL "$ENV{GITHUB_HUNTER_USERNAME}" "" username_is_empty)
string(COMPARE EQUAL "$ENV{GITHUB_HUNTER_TOKEN}" "" password_is_empty)

# binary cache can be uploaded to qdrvm/hunter-binary-cache so others will not build same dependencies twice
if (NOT password_is_empty AND NOT username_is_empty)
    option(HUNTER_RUN_UPLOAD "Upload cache binaries" YES)
    message("Binary cache uploading is ENABLED.")
else ()
    option(HUNTER_RUN_UPLOAD "Upload cache binaries" NO)
    message(AUTHOR_WARNING " Binary cache uploading is DISABLED.
 Define environment variables GITHUB_HUNTER_USERNAME and GITHUB_HUNTER_TOKEN
 for binary cache activation. To generate github token follow the instructions:
 https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens
 Make sure `read:packages` and `write:packages` permissions are granted (step 7 in instructions).")
endif ()

set(HUNTER_PASSWORDS_PATH
    "${CMAKE_CURRENT_LIST_DIR}/passwords.cmake"
    CACHE FILEPATH "Hunter passwords"
)

set(HUNTER_CACHE_SERVERS
    "https://github.com/qdrvm/hunter-binary-cache"
    CACHE STRING "Binary cache server"
)

# https://hunter.readthedocs.io/en/latest/reference/user-variables.html#hunter-use-cache-servers
# set(
#     HUNTER_USE_CACHE_SERVERS NO
#     CACHE STRING "Disable binary cache"
# )

# https://hunter.readthedocs.io/en/latest/reference/user-variables.html#hunter-status-debug
# set(
#     HUNTER_STATUS_DEBUG ON
#     CACHE STRING "Enable output lot of info for debugging"
# )

include(${CMAKE_CURRENT_LIST_DIR}/HunterGate.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/hunter-gate-url.cmake)
