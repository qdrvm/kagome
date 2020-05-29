#!/bin/bash -xe

readonly DIR=$( cd $(dirname $0)/.. ; pwd -P )
INDOCKER_IMAGE="${INDOCKER_IMAGE:?INDOCKER_IMAGE variable is not defined}"
CI_ENV=$(env | awk -F= -v key='^(BRANCH_|BUILD_|HUDSON_|JENKINS_|JOB_|CHANGE_|TRAVIS_|GITHUB_)' '$1 ~ key {print "-e " $1}')
docker run -i --rm \
   --cap-add SYS_PTRACE \
   -v /tmp/cache/hunter:/root/.hunter \
   -v ${DIR}:/workdir \
   -w /workdir \
   -e CTEST_OUTPUT_ON_FAILURE \
   -e CODECOV_TOKEN \
   -e SONAR_TOKEN \
    $CI_ENV \
   "${INDOCKER_IMAGE}" $@
