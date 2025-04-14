#if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
#  hunter_upload_password(
#    REPO_OWNER "qdrvm"
#    REPO "hunter-binary-cache"
#    USERNAME "$ENV{GITHUB_HUNTER_USERNAME}"
#    PASSWORD "$ENV{GITHUB_HUNTER_TOKEN}"
#  )
#else()
#  hunter_upload_password(
#    REPO_OWNER "qdrvm"
#    REPO "hunter-cache"
#    USERNAME "$ENV{GITHUB_HUNTER_USERNAME}"
#    PASSWORD "$ENV{GITHUB_HUNTER_TOKEN}"
#  )
#endif()

hunter_upload_password(
  REPO_OWNER "qdrvm"
  REPO "hunter-binary-cache"
  USERNAME "$ENV{GITHUB_HUNTER_USERNAME}"
  PASSWORD "$ENV{GITHUB_HUNTER_TOKEN}"
)
