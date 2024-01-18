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

def write_file(file:str, text:str):
  filename = file + "-version.txt"
  f = open(filename, "w")
  f.write(file.upper().replace("-", "_") + "_RELEASE=" + text)
  f.close()

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
  print (f"Last Tag: {last_tag}")
  write_file(repo_short_name, last_tag)

def help():
  print("""
    This script: 
      - takes a tag list, 
      - parses tags and finds the latest release tag with semantic version (polkadot-v0.0.0)
      - writes latest tag to file
    Usage:
      python version.py https://github.com/paritytech/polkadot.git
    Result:
      [repo]-version.txt with [REPO]_RELEASE=[version] content
    Example:
      polkadot-sdk-version.txt file with POLKADOT_SDK_RELEASE=v0.1.6
    """)

def main():
  args = sys.argv[1:]
  if "--help" in args:
    help()
    exit()

  if len(args) == 1:
    get_version(args[0])

main()