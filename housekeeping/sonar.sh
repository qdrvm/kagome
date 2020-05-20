#!/bin/bash -xe

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
GIT_HUB_REPOSITORY="${GIT_HUB_REPOSITORY:?GIT_HUB_REPOSITORY variable is not defined}"
GIT_HUB_TOKEN="${GIT_HUB_TOKEN:?GIT_HUB_TOKEN variable is not defined}"
BRANCH_NAME="${BRANCH_NAME:?BRANCH_NAME variable is not defined}"
SONAR_TOKEN="${SONAR_TOKEN:?SONAR_TOKEN variable is not defined}"

sonar_option=""
if [ -n "${CHANGE_ID}" ]; then
  sonar_option="-Dsonar.github.pullRequest=${CHANGE_ID}"
fi

# do analysis by sorabot
sonar-scanner \
  -Dsonar.github.disableInlineComments=true \
  -Dsonar.github.repository="${GIT_HUB_REPOSITORY}" \
  -Dsonar.projectVersion=${BRANCH_NAME} \
  -Dsonar.login=${SONAR_TOKEN} \
  -Dsonar.cxx.jsonCompilationDatabase=${BUILD_DIR}/compile_commands.json \
  -Dsonar.github.oauth=${GIT_HUB_TOKEN} ${sonar_option} \
  -Dsonar.cxx.coverage.reportPath=${BUILD_DIR}/ctest_coverage.xml \
  -Dsonar.cxx.xunit.reportPath=${BUILD_DIR}/xunit/xunit*.xml