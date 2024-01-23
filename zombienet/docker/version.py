import re, sys, subprocess

def list_all_tags_for_remote_git_repo(repo_url):
  result = subprocess.run([
    "git", "ls-remote", "--tags", repo_url
  ], stdout=subprocess.PIPE, text=True)
  output_lines = result.stdout.splitlines()
  tags = [
    line.split("refs/tags/")[-1] for line in output_lines
    if "refs/tags/" in line and "^{}" not in line
  ]
  return tags

def write_file(file: str, text: str):
  version_info = {
    "version": text,
    "short_version": text.replace("polkadot-", ""),
    "numeric_version": re.sub(r'^(polkadot-)?v', '', text),
    "polkadot_format_version": text if "polkadot-" in text else None
  }

  filename = file + "-versions.txt"
  with open(filename, "w") as f:
    for key, value in version_info.items():
      f.write(f"{key}: {value}\n")

def get_last_tag(release_tags):
  def version_key(tag):
    parts = re.sub(r'^(polkadot-)?v', '', tag).split(".")
    return [int(part) for part in parts]
  filtered_tags = [tag for tag in release_tags if re.match(r'^(polkadot-)?v\d+\.\d+\.\d+$', tag)]
  sorted_tags = sorted(filtered_tags, key=version_key)
  return sorted_tags[-1] if sorted_tags else None

def select_release_tags(all_tags):
  tags = []
  for tag in all_tags:
    res = re.search(r'^(polkadot-)?v(\d+\.\d+\.\d+)$', tag)
    if res is not None:
      tags.append(res.group(0))
  return tags

def get_version(repo_url):
  repo_short_name = repo_url.split("/")[-1].split(".")[0]
  all_tags = list_all_tags_for_remote_git_repo(repo_url) #cloned_repo.tags
  print(f"All tags received: {all_tags}")
  release_tags = select_release_tags(all_tags)
  print((f"Filtered release tags: {release_tags}"))
  last_tag = get_last_tag(release_tags)
  print (f"Latest release: {last_tag}")

  write_file(repo_short_name, last_tag)

def help():
  print("""
    This script: 
      - retrieves tags from a remote Git repository,
      - filters and sorts these tags to identify the latest release tag based on semantic versioning (e.g., polkadot-v0.0.0),
      - writes detailed version information to a file.
    
    Usage:
      python version.py <repository_url>
      - <repository_url>: URL of the Git repository to process (e.g., https://github.com/paritytech/polkadot.git).
    
    Result:
      The script generates a file named [repo_short_name]-versions.txt containing key-value pairs of version information:
        - 'version': the latest version tag (e.g., polkadot-v0.1.6),
        - 'short_version': the short version string (e.g., v0.1.6),
        - 'numeric_version': the numeric part of the version (e.g., 0.1.6),
        - 'polkadot_format_version': the complete version tag if it includes 'polkadot-' prefix, otherwise 'None'.
    
    Example:
      For a repository URL https://github.com/paritytech/polkadot.git,
      the script will generate a file named 'polkadot-versions.txt' with content like:
        version: polkadot-v0.1.6
        short_version: v0.1.6
        numeric_version: 0.1.6
        polkadot_format_version: polkadot-v0.1.6
    """)

def main():
  args = sys.argv[1:]
  if "--help" in args:
    help()
    exit()

  if len(args) == 1:
    get_version(args[0])

main()