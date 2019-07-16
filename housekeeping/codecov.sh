#!/bin/bash -xe

bash <(curl -s https://codecov.io/bash) -s $1 -t ${CODECOV_TOKEN}
