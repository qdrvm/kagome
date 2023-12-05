#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
GITHUB_REPOSITORY="${GITHUB_REPOSITORY:?GITHUB_REPOSITORY variable is not defined}"
GITHUB_TOKEN="${GITHUB_TOKEN:?GITHUB_TOKEN variable is not defined}"
SONAR_TOKEN="${SONAR_TOKEN:?SONAR_TOKEN variable is not defined}"
BRANCH_NAME="${BRANCH_NAME:?BRANCH_NAME variable is not defined}"
# For github action we need remove ref prefix
if [[ "$BRANCH_NAME"  == refs/heads/* ]]; then
  BRANCH_NAME="${BRANCH_NAME#refs/heads/}"
elif [[ "$BRANCH_NAME"  == refs/tags/* ]]; then
  BRANCH_NAME="${BRANCH_NAME#refs/tags/}"
elif [[ "$BRANCH_NAME"  == refs/pull/* ]]; then
  BRANCH_NAME="${GITHUB_HEAD_REF}"
fi

sonar_option=""
if [ -n "${CHANGE_ID}" ]; then
  sonar_option="-Dsonar.github.pullRequest=${CHANGE_ID}"
fi

# do analysis by sorabot
sonar-scanner \
  -Dsonar.github.disableInlineComments=true \
  -Dsonar.github.repository="${GITHUB_REPOSITORY}" \
  -Dsonar.projectVersion=${BRANCH_NAME} \
  -Dsonar.login=${SONAR_TOKEN} \
  -Dsonar.cxx.jsonCompilationDatabase=${BUILD_DIR}/compile_commands.json \
  -Dsonar.github.oauth=${GITHUB_TOKEN} ${sonar_option} \
  -Dsonar.cxx.coverage.reportPath=${BUILD_DIR}/ctest_coverage.xml \
  -Dsonar.cxx.xunit.reportPath=${BUILD_DIR}/xunit/xunit*.xml
