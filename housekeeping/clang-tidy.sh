#!/usr/bin/env bash -xe

# this script filters build/compile_commands.json and runs clang-tidy on files changed in the last commit

BUILD_DIR=$(echo "$(cd "$(dirname "$1")"; pwd -P)/$(basename "$1")")

BN=$(dirname $0)
cd ${BN}



# list of cpp files changed in this branch (in comparison to master); tests are ignored
FILES=$(git diff --name-only HEAD..origin/master | grep "cpp" | grep -v "test")
CLANG_TIDY=$(find /usr/local/Cellar/llvm -type f -name clang-tidy | head -n 1)
RUN_CLANG_TIDY=$(find /usr/local/Cellar/llvm -type f -name run-clang-tidy.py | head -n 1)

# filter compile_commands.json
echo ${FILES} | python3 ./filter_compile_commands.py -p ${BUILD_DIR}

python3 ${RUN_CLANG_TIDY} -clang-tidy-binary=${CLANG_TIDY} -p ${BUILD_DIR}
