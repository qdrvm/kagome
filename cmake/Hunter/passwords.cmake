hunter_upload_password(
  # REPO_OWNER + REPO = https://github.com/forexample/hunter-cache
  REPO_OWNER "qdrvm"
  REPO "hunter-cache"

  # USERNAME = warchant
  USERNAME "$ENV{GITHUB_HUNTER_USERNAME}"

  # PASSWORD = GitHub token saved as a secure environment variable
  PASSWORD "$ENV{GITHUB_HUNTER_TOKEN}"
)
