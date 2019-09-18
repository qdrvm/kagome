dockerImage = 'soramitsu/kagome-dev:7'
workerLabel = 'd3-build-agent'
buildDir = "/tmp/build"
repository = "soramitsu/kagome"

def makeBuild(String name, String clangFlags, Closure then) {
  return {
    stage(name) {
      try {
        node(workerLabel) {
          scmVars = checkout scm
          sh(script: "git pull --recurse-submodules")
          sh(script: "git submodule update --init --recursive")
          withCredentials([
            string(credentialsId: 'codecov-token', variable: 'codecov_token'),
            usernamePassword(credentialsId: 'sorabot-github-user', passwordVariable: 'sorabot_password', usernameVariable: 'sorabot_username'),
            string(credentialsId: 'SONAR_TOKEN', variable: 'SONAR_TOKEN')
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
  } // end stage
} // end makeBuild

def makeToolchainBuild(String name, String toolchain) {
  return makeBuild(name, "-DCMAKE_TOOLCHAIN_FILE=${toolchain}", {
    sh(script: "cmake --build ${buildDir} -- -j4")
    sh(script: "cmake --build ${buildDir} --target test")
  })
} // end makeToolchainBuild


def makeCoverageBuild(String name){
  return makeBuild(name, "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-8_cxx17.cmake -DCOVERAGE=ON", {
    sh(script: "cmake --build ${buildDir} -- -j4")

    // submit coverage
    sh(script: "cmake --build ${buildDir} --target ctest_coverage")
    withCredentials([
      string(credentialsId: 'codecov-token', variable: 'codecov_token')
    ]){
      sh(script: "./housekeeping/codecov.sh ${buildDir} ${codecov_token}")
    }
  })
}

def makeClangTidyBuild(String name){
  return makeBuild(name, "", {
    // run clang-tidy
    sh(script: "housekeeping/clang-tidy.sh ${buildDir}")
  })
}

def makeAsanBuild(String name){
  return makeBuild(name, "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-8_cxx17.cmake -DASAN=ON", {
    sh(script: "cmake --build ${buildDir} -- -j4")
    sh(script: "cmake --build ${buildDir} --target test")
  })
}

def makeSonarScanner(String name){
  return makeBuild(name, "", {
      sonar_option = ""
      if (env.CHANGE_ID != null) {
        sonar_option = "-Dsonar.github.pullRequest=${env.CHANGE_ID}"
      }
      // do analysis by sorabot
      sh """
        sonar-scanner \
          -Dsonar.github.disableInlineComments=true \
          -Dsonar.github.repository='${repository}' \
          -Dsonar.login=${SONAR_TOKEN} \
          -Dsonar.github.oauth=${sorabot_password} ${sonar_option}
      """
  })
}

node(workerLabel){
  try {
    stage("checks") {
      def builds = [:]
      // clang-tidy fails. see https://bugs.llvm.org/show_bug.cgi?id=42648
      // builds["clang-tidy"] = makeClangTidyBuild("clang-tidy")
      builds["gcc-8 ASAN No Toolchain"] = makeAsanBuild("gcc-8 ASAN No Toolchain")
      builds["clang-8 TSAN"] = makeToolchainBuild("clang-8 TSAN", "cmake/san/clang-8_cxx17_tsan.cmake")
      builds["clang-8 UBSAN"] = makeToolchainBuild("clang-8 UBSAN", "cmake/san/clang-8_cxx17_ubsan.cmake")
      builds["gcc-8 coverage"] = makeCoverageBuild("gcc-8 coverage")
      builds["sonar Scanner"] = makeSonarScanner("sonar Scanner")

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
