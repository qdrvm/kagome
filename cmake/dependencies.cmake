# https://docs.hunter.sh/en/latest/packages/

# https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
# exports GTest::main GTest::gtest targets
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
