#!/bin/bash

cd `dirname $0`

KAGOME_PREFIX=/opt/docker/kagome

DOCKERHUB_PUBLISH=0 # Set '1' for ability to publish containers
DOCKERHUB_USERNAME=kagome
DOCKERHUB_PASSWORD="******"
DOCKERHUB_IMAGE=${DOCKERHUB_USERNAME}/kagome

CODE=kagome

BUILD_ENVIRONMENT_IMAGE=${CODE}_build_environment
BUILD_IMAGE=${CODE}_build
BUILD_CONTAINER=${CODE}_build

BINARIES_IMAGE=${CODE}
BINARIES_CONTAINER=${CODE}

DATABASE_CONTAINER=${CODE}_db

NETWORK=${CODE}_net

_build_env()
{
  local REBUILD
  local NOCACHE

  for ARG in "$@"; do
    if [ "$ARG" = "rebuild" ]; then
      REBUILD=1
    elif [ "$ARG" = "nocache" ]; then
      NOCACHE="--no-cache"
    fi
  done

  OLD_IMG=`docker images --quiet ${BUILD_ENVIRONMENT_IMAGE} 2>/dev/null`
	if [ -z "$OLD_IMG" ]
	then
		echo "Build new '${BUILD_ENVIRONMENT_IMAGE}' image"
	elif [ "$REBUILD" = "1" ]
	then
	  if [ -z "$NOCACHE" ]; then
  		echo "Partial rebuild '${BUILD_ENVIRONMENT_IMAGE}' image"
	  else
  		echo "Total rebuild '${BUILD_ENVIRONMENT_IMAGE}' image"
	  fi
	else
		echo "Image '${BUILD_ENVIRONMENT_IMAGE}' already exists"
		return 0
	fi

	rm -Rf context
	mkdir -p context

	cp -a kagome_build-entrypoint.sh context/docker-entrypoint.sh
	chmod u=rwx,g=rx,o=rx context/docker-entrypoint.sh

	docker image build \
		${NOCACHE} \
		--rm \
		--tag ${BUILD_ENVIRONMENT_IMAGE} \
		--file Dockerfile_${BUILD_ENVIRONMENT_IMAGE} \
		context

	rm -Rf context
}

_publish_env()
{
	echo
	echo "Publish '${BUILD_ENVIRONMENT_IMAGE}' image"

  if [ "${DOCKERHUB_PUBLISH}" -ne "1" ]
  then
  	echo "Not enabled!" > 1>&2
  	echo "Set DOCKERHUB_PUBLISH to '1' and setup passwors in DOCKERHUB_PASSWORD before"
  	exit 1
  fi

	if [ -z "`docker images --quiet ${BUILD_ENVIRONMENT_IMAGE} 2>/dev/null`" ]
	then
		echo "Image ${BUILD_ENVIRONMENT_IMAGE} not found. Make it by command 'build_env'" >&2
		exit 1
	fi

	if [ $(docker-credential-secretservice list | grep ":\"${DOCKERHUB_USERNAME}\"" | wc -l) = "0" ]
	then
		echo "Check and save credentials"
		echo ${DOCKERHUB_PASSWORD} | \
		docker login \
			--username ${DOCKERHUB_USERNAME} \
			--password-stdin
	fi

	echo "Set tag ${DOCKERHUB_IMAGE}:build_environment for ${BUILD_CONTAINER}_environment"
	docker tag ${BUILD_CONTAINER}_environment ${DOCKERHUB_IMAGE}:build_environment

	echo "Push ${DOCKERHUB_IMAGE}:build_environment"
	docker push ${DOCKERHUB_IMAGE}:build_environment
}

_prepare()
{
  local REBUILD
  local NOCACHE

  for ARG in "$@"; do
    if [ "$ARG" = "rebuild" ]; then
      REBUILD=1
    elif [ "$ARG" = "nocache" ]; then
      NOCACHE="--no-cache"
    fi
  done

	if [ -z "`docker images --quiet ${BUILD_IMAGE} 2>/dev/null`" ]
	then
		echo "Build new '${BUILD_IMAGE}' image"
		NOCACHE=
	elif [ "$1" = "rebuild" ]
	then
	  if [ -z "$NOCACHE" ]; then
  		echo "Partial rebuild '${BUILD_IMAGE}' image"
	  else
	  	echo "Total rebuild '${BUILD_IMAGE}' image"
	  fi
	else
		echo "Image '${BUILD_IMAGE}' is ready"
		return 0
	fi

	rm -Rf context
	mkdir -p context

	cp -a kagome_build-_ssh context/.ssh
	chmod -R u+rw,g-w,o-w context/.ssh
#	chmod g=,o= context/.ssh/id_rsa
	cp -a kagome_build-entrypoint.sh context/docker-entrypoint.sh
	chmod u=rwx,g=rx,o=rx context/docker-entrypoint.sh

	docker image build \
		${NOCACHE} \
		--tag ${BUILD_CONTAINER} \
		--file Dockerfile_kagome_build \
		context

	rm -Rf context
}

_build()
{
	_prepare "$@"

	if [ -z "`docker inspect -f '{{.State.Running}}' ${BUILD_CONTAINER} 2>/dev/null`" ]
	then
		echo "Create and start '${BUILD_CONTAINER}' container"
		docker container run \
			--name ${BUILD_CONTAINER} \
			--tty \
			--interactive \
			${BUILD_CONTAINER}
	elif [ "`docker inspect -f '{{.State.Running}}' ${BUILD_CONTAINER} 2>/dev/null`" = "false" ]
	then
		echo "Start '${BUILD_CONTAINER}' container"
		docker container start \
			--attach \
			--interactive \
			${BUILD_CONTAINER}
	elif [ "`docker inspect -f '{{.State.Running}}' ${BUILD_CONTAINER} 2>/dev/null`" = "true" ]
	then
		echo "Restart '${BUILD_CONTAINER}' container"
		docker container stop ${BUILD_CONTAINER}
		while [ "`docker inspect -f '{{.State.Running}}' ${BUILD_CONTAINER}`" = "true" ]
		do
			sleep 1
		done
		docker container start \
			--attach \
			--interactive \
			${BUILD_CONTAINER}
	fi

	echo "Make context for 'kagome' image"

	rm -Rf context
	mkdir -p context

  docker cp ${BUILD_CONTAINER}:/root/kagome/build/node/kagome_full/kagome_full context/kagome_full 2>/dev/null \
		|| (echo "Can't fetch kagome_full. Terminate." >&2 && false) || exit 1
  docker cp ${BUILD_CONTAINER}:/root/kagome/build/node/kagome_syncing/kagome_syncing context/kagome_syncing 2>/dev/null \
		|| (echo "Can't fetch kagome_syncing. Terminate." >&2 && false) || exit 1
	docker cp -a ${BUILD_CONTAINER}:/root/kagome/node/config context/config 2>/dev/null \
		|| (echo "Can't fetch config. Terminate." >&2 && false) || exit 1

	echo
	echo "Stop '${BUILD_CONTAINER}' container"

	docker container stop ${BUILD_CONTAINER}

	echo
	echo "Make '${BINARIES_IMAGE}_local' image"
	docker image build \
		${NOCACHE} \
		--rm \
		--tag ${BINARIES_IMAGE}_local \
		--file Dockerfile_kagome \
		context

	docker tag ${BINARIES_IMAGE}_local ${BINARIES_IMAGE}

	rm -Rf context
}

_publish()
{
	echo
	echo "Publish '${BINARIES_IMAGE}' image"

  if [ "${DOCKERHUB_PUBLISH}" -ne "1" ]
  then
  	echo "Not enabled!" > 1>&2
  	echo "Set DOCKERHUB_PUBLISH to '1' and setup passwors in DOCKERHUB_PASSWORD before"
  	exit 1
  fi

	if [ $(docker-credential-secretservice list | grep ":\"${DOCKERHUB_USERNAME}\"" | wc -l) = "0" ]
	then
		echo "Check and save credentials"
		echo ${DOCKERHUB_PASSWORD} | \
		docker login \
			--username ${DOCKERHUB_USERNAME} \
			--password-stdin
	fi

	echo "Set tag ${DOCKERHUB_IMAGE}:$1 for ${BINARIES_IMAGE}_local"
	docker tag ${BINARIES_IMAGE}_local ${DOCKERHUB_IMAGE}:$1

	echo "Push ${DOCKERHUB_IMAGE}:$1"
	docker push ${DOCKERHUB_IMAGE}:$1
}

_usage()
{
	echo "Usage:"
	echo "	$0 build_env [OPTIONS]  - build container with evironment for build kagome binaries"
	echo "	$0 publish_env          - publish prev docker image on dockerhub"
	echo "	$0 build [OPTIONS]      - build binaries and docker image"
	echo "	$0 publish              - publish prev docker image on dockerhub"
#	echo "	$0 makedeb     - make debian package"
  echo "OPTIONS is one of follow"
  echo "  rebuild - for force rebuild docker container"
  echo "  nocache - for avoid using of docker cache"
	exit 1
}

for ARG in "$@"; do
	if [ "$ARG" = "build" ]; then BUILD=1
	elif [ "$ARG" = "publish" ]; then PUBLISH=1
	elif [ "$ARG" = "build_env" ]; then BUILD_ENV=1
	elif [ "$ARG" = "publish_env" ]; then PUBLISH_ENV=1
	elif [ "$ARG" = "makedeb" ]; then MAKEDEB=1
	elif [ "$ARG" = "rebuild" ]; then REBUILD=1
	elif [ "$ARG" = "nocache" ]; then NOCACHE=1
	else echo "Unknown option '$ARG'"; echo; _usage; exit 1; fi
done

USAGE=1

if [ "$REBUILD" = "1" ]; then
  ADD_OPTIONS="${ADD_OPTIONS} rebuild"
fi

if [ "$NOCACHE" = "1" ]; then
  ADD_OPTIONS="${ADD_OPTIONS} nocache"
fi

if [ "$BUILD_ENV" = "1" ]; then
	_build_env $ADD_OPTIONS
	USAGE=0
fi

if [ "$PUBLISH_ENV" = "1" ]; then
	_publish_env
	USAGE=0
fi

if [ "$BUILD" = "1" ]; then
	_build $ADD_OPTIONS
	USAGE=0
fi

if [ "$PUBLISH" = "1" ]; then
	_publish latest
	USAGE=0
fi

if [ "$MAKEDEB" = "1" ]; then
	. makedeb.sh
	USAGE=0
fi

if [ "$USAGE" = "1" ]; then
	_usage
fi

exit 0
