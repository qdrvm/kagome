#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

readonly DIR=$( cd $(dirname $0)/.. ; pwd -P )
INDOCKER_IMAGE="${INDOCKER_IMAGE:?INDOCKER_IMAGE variable is not defined}"
CI_ENV=$(env | awk -F= -v key='^(BRANCH_|BUILD_|HUDSON_|JENKINS_|JOB_|CHANGE_|TRAVIS_|GITHUB_)' '$1 ~ key {print "-e " $1}')

render_version() {
sedStr="
  s!%%INDOCKER_IMAGE%%!$INDOCKER_IMAGE!g;
"
sed -r "$sedStr" $1
}

render_version $DIR/housekeeping/indocker/Dockerfile.template > $DIR/Dockerfile

docker build -t soramitsu/kagome:local-dev $DIR

rm $DIR/Dockerfile

docker run -i --rm \
   --cap-add SYS_PTRACE \
   -v /tmp/cache/hunter:/root/.hunter \
   -w /workdir \
   -e CTEST_OUTPUT_ON_FAILURE \
   -e CODECOV_TOKEN \
   -e SONAR_TOKEN \
   $CI_ENV \
   soramitsu/kagome:local-dev
