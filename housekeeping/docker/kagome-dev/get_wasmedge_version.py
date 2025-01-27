import re
import sys


def extract_version(config_file_path, package_name):
    with open(config_file_path, 'r') as file:
        content = file.read()

    version_pattern = re.compile(r'hunter_config\(\s*' + re.escape(package_name) + r'\s*.*?URL\s+.*?/(?:heads|tags)/([^\s/]+)\.zip', re.DOTALL)
    version_match = version_pattern.search(content)

    version = version_match.group(1) if version_match else None
    return version


def extract_sha(cmakelists_file_path):
    with open(cmakelists_file_path, 'r') as file:
        content = file.read()

    sha_pattern = re.compile(r'set\s*\(\s*WASMEDGE_ID\s*([a-fA-F0-9]+)\s*\)')
    sha_match = sha_pattern.search(content)

    sha = sha_match.group(1)[:7] if sha_match else None
    return sha


if __name__ == "__main__":
    config_file_path = '../../../cmake/Hunter/config.cmake'
    cmakelists_file_path = '../../../CMakeLists.txt'
    package_name = 'WasmEdge'

    version = extract_version(config_file_path, package_name)

    sha = extract_sha(cmakelists_file_path)

    if version and sha:
        print(f"{version}-{sha}")
    else:
        print("Failed to extract version and SHA")
