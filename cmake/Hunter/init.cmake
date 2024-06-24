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

include(${CMAKE_CURRENT_LIST_DIR}/HunterGate.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/hunter-gate-url.cmake)
