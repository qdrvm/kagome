build:
	mkdir build && cd build && \
		cmake -DCMAKE_BUILD_TYPE=Release .. && \
		make kagome -j $(shell expr $(shell nproc 2>/dev/null || sysctl -n hw.ncpu) / 2 + 1 )

docker: 
	INDOCKER_IMAGE=qdrvm/kagome-dev:minideb BUILD_DIR=build BUILD_THREADS=$(shell expr $(shell nproc 2>/dev/null || sysctl -n hw.ncpu) + 1 ) ./housekeeping/indocker.sh ./housekeeping/make_build.sh


command:
	INDOCKER_IMAGE=qdrvm/kagome-dev:minideb ./housekeeping/indocker.sh ${args}


release:
	INDOCKER_IMAGE=qdrvm/kagome-dev:minideb BUILD_DIR=build ./housekeeping/indocker.sh ./housekeeping/docker/release/makeRelease.sh


release_docker:
	VERSION=0.0.1 BUILD_DIR=build BUILD_TYPE=Release ./housekeeping/docker/kagome/build_and_push.sh


debug_docker:
	VERSION=0.0.1 BUILD_DIR=build BUILD_TYPE=Debug ./housekeeping/docker/kagome/build_and_push.sh

custom_docker:
	VERSION=0.0.1 BUILD_TYPE=Custom ./housekeeping/docker/kagome/build_and_push.sh

clear:
	rm -rf build
