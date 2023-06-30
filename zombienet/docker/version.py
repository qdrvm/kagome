import re, sys, git

def write_file(file:str, text:str):
  filename = file + "-version.txt"
  f = open(filename, "w")
  f.write(file.upper() + "_RELEASE=" + text)
  f.close()

def get_last_tag(release_tags):
  int_tags = []
  for i in range(len(release_tags)):
    temp = release_tags[i].split(".")
    int_tags.append([int(temp[0]),int(temp[1]),int(temp[2])])
  last_tag_int = sorted(int_tags)[-1]
  last_tag_str = "v" + str(last_tag_int[0]) + "." + \
                       str(last_tag_int[1]) + "." + \
                       str(last_tag_int[2])
  return last_tag_str

def select_release_tags(all_tags):
  tags = []
  for tag in all_tags:
    res = re.search(r'^v([\d]*.[\d]*.[\d]*)$', tag.name)
    if res != None:
      tags.append(res.group(1))
  return tags

def get_version(repo_name):
  git.Git(".").clone(repo_name)
  repo_short_name = repo_name.split("/")[-1].split(".")[0]
  cloned_repo = git.Repo(repo_short_name)
  all_tags = cloned_repo.tags
  release_tags = select_release_tags(all_tags)
  last_tag = get_last_tag(release_tags)
  write_file(repo_short_name, last_tag)

def help():
  print("""
    This script: 
      - takes a repo, 
      - clones the repo
      - parses tags and finds the latest release tag with semantic version (v0.0.0)
      - writes latest tag to file
    Usage:
      python version.py https://github.com/paritytech/polkadot.git
    Result:
      [repo]-version.txt with [REPO]_RELEASE=[version] content
    Example:
      polkadot-version.txt file with POLKADOT_RELEASE=v0.9.43 content for 26.06.2023
    """)

def main():
  args = sys.argv[1:]
  if "--help" in args:
     help()
     exit()

  if len(args) == 1:
    get_version(args[0])
  
main()