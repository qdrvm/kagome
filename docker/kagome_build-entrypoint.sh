#!/bin/sh

GIT_HEAD=master
if [ -n "${1}" ]
then
  GIT_HEAD="${1}"
fi

cd /root >/dev/null

LOGILE=/root/kagome-build.log

touch ${LOGILE}
truncate -s0 ${LOGILE}

echo "Build kagome binaries"
echo -n "Start at " >> ${LOGILE}
date >> ${LOGILE}

if [ -d /root/kagome/.git ]
then
	echo -n "Update repository "
	cd /root/kagome >/dev/null
	git tag -l | xargs git tag -d >> ${LOGILE} 2>&1
	git fetch >> ${LOGILE} 2>&1
	if [ ! $? ]
	then
    echo "- FAIL" &&
	  echo "Problem at fetch from repository" >&2
	  exit 1
	fi
else
	rm -Rf /root/kagome
	echo -n "Cloning repository "
#	git clone git@github.com:soramitsu/kagome.git >> ${LOGILE} 2>&1
	git clone https://github.com/soramitsu/kagome.git >> ${LOGILE} 2>&1
	if [ ! $? ]
	then
    echo "- FAIL" &&
	  echo "Problem at clone repository" >&2
	  exit 1
	fi
	cd /root/kagome >/dev/null
fi
git reset --hard origin/master >> ${LOGILE} 2>&1
#git reset --hard feature/demo-preparation >> ${LOGILE} 2>&1
if [ ! $? ]
then
  echo "- FAIL" &&
  echo "Problem at reset to master" >&2
  exit 1
fi
echo "- OK"

echo -n "Update submodules "
git submodule update --init --recursive >> ${LOGILE} 2>&1
if [ ! $? ]
then
  echo "- FAIL" &&
  echo "Problem at update submodules" >&2
  exit 1
fi
echo "- OK"

mkdir -p /root/kagome/build >> ${LOGILE} 2>&1 \
  || (echo "Can't create directory of build" >&2 && exit 1)
cd /root/kagome/build >/dev/null

#export GITHUB_HUNTER_USERNAME=xDimon
#export GITHUB_HUNTER_TOKEN=39145ad5c93a36f7ecaf5979bf3e56d81e3aecad

echo -n "Configure build system "
cmake -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release .. -j >> ${LOGILE} 2>&1
if [ ! $? ]
then
  echo "- FAIL" &&
  echo "Can't configure build system" >&2
  exit 1
fi

####### BEGIN: fix for sr25519
      ln /root/.hunter/_Base/a345345/e513020/900f15b/Install/lib64/libsr25519crust.a /root/.hunter/_Base/a345345/e513020/900f15b/Install/lib/libsr25519crust.a > /dev/null
      cmake -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release .. >> ${LOGILE} 2>&1
      if [ ! $? ]
      then
        echo "- FAIL" &&
        echo "Can't configure build system (after sr25519 fix)" >&2
        exit 1
      fi
####### END: fix for sr25519

echo "- OK"

echo "Build binaries..."
cmake --build . --target all -- -j 2>&1 | tee -a ${LOGILE} | egrep "\[\s*[0-9]+%\]" | sed -e 's/^\[[ ]*\([0-9]+\)%\].*/\1/'
if [ ! $? ]
then
  echo "Build binaries - FAIL"
  echo "Can't build binaries" >&2
  exit 1
fi
echo "Build binaries - OK"

strip /root/kagome/build/node/kagome_full/kagome_full > /dev/null
strip /root/kagome/build/node/kagome_syncing/kagome_syncing > /dev/null

echo -n "Finish at " >> ${LOGILE}
date >> ${LOGILE}

echo

sleep 1

exit 0
