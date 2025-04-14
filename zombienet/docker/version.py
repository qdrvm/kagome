import re
import subprocess
import sys


def list_all_tags_for_remote_git_repo(repo_url):
    result = subprocess.run([
        "git", "ls-remote", "--tags", repo_url
    ], stdout=subprocess.PIPE, text=True, timeout=60)
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
        "numeric_version": re.sub(r'^(polkadot-)?stable', '', text),
        "polkadot_format_version": text if "polkadot-" in text else None
    }

    filename = file + "-versions.txt"
    with open(filename, "w") as f:
        for key, value in version_info.items():
            f.write(f"{key}: {value}\n")


def get_last_tag_stable(release_tags):
    def version_key(tag):
        stable_part = re.search(r'stable(\d+)(?:-(\d+))?', tag)
        if stable_part:
            major = int(stable_part.group(1))
            minor = int(stable_part.group(2)) if stable_part.group(2) else 0
            return [major, minor]
        return [0, 0]

    filtered_tags = [tag for tag in release_tags if re.match(r'^polkadot-stable\d+(-\d+)?$', tag)]
    sorted_tags = sorted(filtered_tags, key=version_key)
    return sorted_tags[-1] if sorted_tags else None


def select_release_tags_stable(all_tags):
    tags = []
    for tag in all_tags:
        res = re.search(r'^polkadot-stable\d+(-\d+)?$', tag)
        if res is not None:
            tags.append(res.group(0))
    return tags


def get_version(repo_url, version=None):
    repo_short_name = repo_url.split("/")[-1].split(".")[0]

    if version:
        print(f"Using provided version: {version}")
        write_file(repo_short_name, version)
    else:
        all_tags = list_all_tags_for_remote_git_repo(repo_url)
        #print(f"All tags received: {all_tags}")

        release_tags_stable = select_release_tags_stable(all_tags)
        #print(f"Filtered stable release tags: {release_tags_stable}")
        last_tag_stable = get_last_tag_stable(release_tags_stable)
        print(f"Latest stable release: {last_tag_stable}")

        write_file(repo_short_name, last_tag_stable)


def help():
    print("""
    This script: 
      - retrieves tags from a remote Git repository,
      - filters and sorts these tags to identify the latest stable release tag (e.g., polkadot-stable2409),
      - or uses a provided version if specified,
      - writes detailed version information to a file.

    Usage:
      python version.py <repository_url> [version]
      - <repository_url>: URL of the Git repository to process (e.g., https://github.com/paritytech/polkadot.git).
      - [version]: Optional, specify a version to use instead of fetching the latest from the repo.

    Result:
      The script generates a file named [repo_short_name]-versions.txt containing key-value pairs of version information:
        - 'version': the specified or latest stable version tag (e.g., polkadot-stable2409),
        - 'short_version': the short version string (e.g., stable2409),
        - 'numeric_version': the numeric part of the version (e.g., 2409),
        - 'polkadot_format_version': the complete version tag if it includes 'polkadot-' prefix.
    """)


def main():
    args = sys.argv[1:]
    if "--help" in args:
        help()
        exit()

    if len(args) == 1:
        get_version(args[0])
    elif len(args) == 2:
        get_version(args[0], args[1])
    else:
        print("Invalid usage. Use --help for more information.")


main()
