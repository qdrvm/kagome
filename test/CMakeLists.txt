add_subdirectory(core)

addtest(example1 part1.cpp)
addtest_part(example1 part2.cpp)

addtest(example2 part1.cpp part2.cpp)

add_subdirectory(binaryen)
