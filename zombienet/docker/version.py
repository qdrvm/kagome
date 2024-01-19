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

def write_file(file: str, text: str, version_format="long"):
  if version_format == "short":
    filename = file + "-short-version.txt"
    content = text.replace("polkadot-", "")
  elif version_format == "polkadot":
      if "polkadot-" in text:
          filename = file + "-polkadot-version.txt"
          content = text
      else:
          return
  else:
    filename = file + "-version.txt"
    content = file.upper().replace("-", "_") + "_RELEASE=" + text

  with open(filename, "w") as f:
    f.write(content)

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

  write_file(repo_short_name, last_tag)  # Long format
  write_file(repo_short_name, last_tag, version_format="short")  # Short format
  write_file(repo_short_name, last_tag, version_format="polkadot")  # Polkadot format

def help():
  print("""
    This script: 
      - takes a tag list, 
      - parses tags and finds the latest release tag with semantic version (polkadot-v0.0.0)
      - writes latest tag to files (short, long, with environment variable)
    Usage:
      python version.py https://github.com/paritytech/polkadot.git
    Result:
      [repo]-version.txt with [REPO]_RELEASE=[long-version] content
      [repo]-polkadot-version.txt with [long-version] content
      [repo]-short-version.txt with [short-version] content
    Example:
      polkadot-sdk-version.txt file with POLKADOT_SDK_RELEASE=polkadot-v0.1.6
      polkadot-polkadot-version.txt with polkadot-v0.1.6 content
      polkadot-short-version.txt with v0.1.6 content
    """)

def main():
  args = sys.argv[1:]
  if "--help" in args:
    help()
    exit()

  if len(args) == 1:
    get_version(args[0])

main()