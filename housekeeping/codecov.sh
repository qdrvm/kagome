#!/bin/bash -xe

if [ -z "$1" ]; then
    echo "1 arg is empty"
    exit 1
fi

if [ -z "$2" ]; then
    echo "2 arg is empty"
    exit 2
fi

cd $1
bash <(curl -s https://codecov.io/bash) -t $2
