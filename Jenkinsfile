dockerImage = 'soramitsu/kagome-dev:6'
workerLabel = 'd3-build-agent'
buildDir = "/tmp/build"


def makeBuild(String clangFlags, Closure then) {
  return {
    try {
      node(workerLabel) {
        scmVars = checkout scm
        sh(script: "git pull --recurse-submodules")
        sh(script: "git submodule update --init --recursive")
        withCredentials([
          string(credentialsId: 'codecov-token', variable: 'codecov_token'),
          usernamePassword(credentialsId: 'sorabot-github-user', passwordVariable: 'sorabot_password', usernameVariable: 'sorabot_username')
        ]) {
          docker.image(dockerImage).inside('--cap-add SYS_PTRACE') {

            // define env to enable hunter cache
            withEnv([
              "CTEST_OUTPUT_ON_FAILURE=1",
              "GITHUB_HUNTER_USERNAME=${sorabot_username}",
              "GITHUB_HUNTER_TOKEN=${sorabot_password}",
              "CODECOV_TOKEN=${codecov_token}"
            ]) {
              sh(script: "cmake . -B${buildDir} ${clangFlags}")
              // continue execution
              then()
            } // end withEnv
          } // end docker.image
        } // end withCredentials
      } // end node
    } catch(Exception e) {
      currentBuild.result = 'FAILURE'
      error(e)
    } // end catch
  } // end return
} // end makeBuild

def makeToolchainBuild(String toolchain) {
  return makeBuild("-DCMAKE_TOOLCHAIN_FILE=${toolchain}", {
    sh(script: "cmake --build ${buildDir} -- -j4")
    sh(script: "cmake --build ${buildDir} --target test")
  })
} // end makeToolchainBuild


def makeCoverageBuild(){
  return makeBuild("-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-8_cxx17.cmake -DCOVERAGE=ON", {
    sh(script: "cmake --build ${buildDir} -- -j4")

    // submit coverage
    sh(script: "cmake --build ${buildDir} --target ctest_coverage")
    sh(script: "./housekeeping/codecov.sh ${buildDir}")
  })
}

def makeClangTidyBuild(){
  return makeBuild("", {
    // run clang-tidy
    sh(script: "housekeeping/clang-tidy.sh ${buildDir}")
  })
}

def makeAsanBuild(){
  return makeBuild("-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-8_cxx17.cmake -DASAN=ON", {
    sh(script: "cmake --build ${buildDir} -- -j4")
    sh(script: "cmake --build ${buildDir} --target test")
  })
}


node(workerLabel){
  try {
      stage("checks") {
        def builds = [:]
        builds["clang-tidy"] = makeClangTidyBuild()
        builds["gcc-8 ASAN No Toolchain"] = makeAsanBuild()
        builds["clang-8 TSAN"] = makeToolchainBuild("cmake/san/clang-8_cxx17_tsan.cmake")
        builds["clang-8 UBSAN"] = makeToolchainBuild("cmake/san/clang-8_cxx17_ubsan.cmake")
        builds["gcc-8 coverage"] = makeCoverageBuild()

        parallel(builds)
      }
  } catch(Exception e) {
      currentBuild.result = 'FAILURE'
      error(e)
  }
      finally {
      cleanWs()
  } // end finally
} // end node
