dockerImage = 'soramitsu/kagome-dev:8'
workerLabel = 'd3-build-agent'
buildDir = "build"
repository = "soramitsu/kagome"

def makeBuild(String name, Closure then) {
  return {
    stage(name) {
      try {
        node(workerLabel) {
          scmVars = checkout scm
          withCredentials([
            string(credentialsId: 'codecov-token', variable: 'CODECOV_TOKEN'),
            usernamePassword(credentialsId: 'sorabot-github-user', passwordVariable: 'GITHUB_TOKEN', usernameVariable: 'GITHUB_USERNAME'),
            string(credentialsId: 'SONAR_TOKEN', variable: 'SONAR_TOKEN')
          ]) {
              withEnv([
                "CTEST_OUTPUT_ON_FAILURE=1",
                "GITHUB_HUNTER_USERNAME=${GITHUB_USERNAME}",
                "GITHUB_HUNTER_TOKEN=${GITHUB_TOKEN}",
                "BUILD_DIR=${buildDir}",
                "GITHUB_REPOSITORY=${repository}",
                "INDOCKER_IMAGE=${dockerImage}"
              ]) {
                then()
              } // end withEnv
          } // end withCredentials
        } // end node
      } catch(Exception e) {
        currentBuild.result = 'FAILURE'
        error(e)
      } // end catch
    } // end return
  } // end stage
} // end makeBuild

def makeToolchainBuild(String name, String toolchain) {
  return makeBuild(name, {
    sh(script: "./housekeeping/indocker.sh ./housekeeping/makeBuild.sh -DCMAKE_TOOLCHAIN_FILE=${toolchain} ")
  })
}

def makeCoverageBuild(String name){
  return makeBuild(name, {
    sh "BUILD_TARGET=ctest_coverage ./housekeeping/indocker.sh ./housekeeping/makeBuild.sh -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-8_cxx17.cmake -DCOVERAGE=ON"
    // submit coverage
    sh "./housekeeping/indocker.sh ./housekeeping/codecov.sh ${buildDir} ${CODECOV_TOKEN}"
    sh "./housekeeping/indocker.sh ./housekeeping/sonar.sh"
  })
}

def makeClangTidyBuild(String name){
  return makeBuild(name, {
    // build only generated files, so clang-tidy will work correctly
    sh(script: "BUILD_TARGET=generated ./housekeeping/indocker.sh ./housekeeping/makeBuild.sh")
    sh(script: "./housekeeping/indocker.sh ./housekeeping/clang-tidy.sh ${buildDir}")
  })
}

def makeAsanBuild(String name){
  return makeBuild(name, {
    sh(script: "./housekeeping/indocker.sh ./housekeeping/makeBuild.sh -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-9_cxx17.cmake -DASAN=ON")
  })
}

node(workerLabel){
  try {
    stage("checks") {
      def builds = [:]
      // clang-tidy fails. see https://bugs.llvm.org/show_bug.cgi?id=42648
      //builds["clang-tidy"] = makeClangTidyBuild("clang-tidy")
      //builds["gcc-9 ASAN No Toolchain"] = makeAsanBuild("gcc-9 ASAN No Toolchain")
      //builds["clang-8 TSAN"] = makeToolchainBuild("clang-8 TSAN", "cmake/san/clang-8_cxx17_tsan.cmake")
      //builds["clang-8 UBSAN"] = makeToolchainBuild("clang-8 UBSAN", "cmake/san/clang-8_cxx17_ubsan.cmake")
      //builds["gcc-8 coverage/sonar"] = makeCoverageBuild("gcc-8 coverage/sonar")

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
