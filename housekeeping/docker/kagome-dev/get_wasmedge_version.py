import re
import sys

def extract_version_and_sha(config_file_path, package_name):
    with open(config_file_path, 'r') as file:
        content = file.read()

    version_pattern = re.compile(r'hunter_config\(\s*' + re.escape(package_name) + r'\s*.*?URL\s+.*?/tags/([^\s/]+)\.zip', re.DOTALL)
    sha_pattern = re.compile(r'hunter_config\(\s*' + re.escape(package_name) + r'\s*.*?SHA1\s+(\w+)', re.DOTALL)

    version_match = version_pattern.search(content)
    sha_match = sha_pattern.search(content)

    version = version_match.group(1) if version_match else None
    sha = sha_match.group(1)[:7] if sha_match else None

    return version, sha

if __name__ == "__main__":
    config_file_path = '../../../cmake/Hunter/config.cmake'
    package_name = 'WasmEdge'

    version, sha = extract_version_and_sha(config_file_path, package_name)
    print(f"{version}-{sha}")