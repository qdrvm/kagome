/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <sys/socket.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <libp2p/common/final_action.hpp>
#include <system_error>
#include <type_traits>

#include "parachain/pvf/kagome_pvf_worker.hpp"

#include "testutil/outcome.hpp"

using namespace kagome::parachain;

template <typename... Ts>
void test_syscall_works(int call, Ts... args) {
  EXPECT_EXIT(([=]() {
                std::ignore = ::syscall(call, args...);
                std::exit(0);
              }()),
              testing::ExitedWithCode(0),
              "");
}

template <typename... Ts>
void test_syscall_fails(int call, Ts... args) {
  EXPECT_EXIT(([=]() {
                EXPECT_OUTCOME_TRUE_1(enableSeccomp());
                std::ignore = ::syscall(call, args...);
                std::exit(0);
              }()),
              testing::KilledBySignal(SIGSYS),
              "");
}

template <typename... Ts>
void test_syscall(int call, Ts... args) {
  test_syscall_works(call, args...);
  test_syscall_fails(call, args...);
}

TEST(SecureMode, SeccompWorks) {
  test_syscall(SYS_socket, AF_INET, SOCK_STREAM, 0);
  int fds[2]{};
  test_syscall(SYS_socketpair, AF_INET, SOCK_STREAM, 0, fds);
  test_syscall(SYS_connect, 0, nullptr, 0);

#if __has_include(<linux/io_uring.h>)
  test_syscall(SYS_io_uring_setup, 0, nullptr);
  test_syscall(SYS_io_uring_enter, 0, 0, 0, 0, nullptr);
  test_syscall(SYS_io_uring_register, 0, 0, nullptr, 0);
#endif
}

TEST(SecureMode, ChangeRootWorks) {
  // since death tests use fork(), landlock restrictions will not leak to other
  // tests in the suite
  EXPECT_EXIT(([=]() {
                libp2p::common::FinalAction final = []() { std::exit(0); };
                auto dir = std::filesystem::temp_directory_path()
                         / "kagome_secure_mode_test/chroot";
                std::filesystem::create_directories(dir);
                ASSERT_OUTCOME_SUCCESS_TRY(changeRoot(dir));
                ASSERT_EQ(std::filesystem::current_path(), "/");
                ASSERT_EQ(
                    std::distance(std::filesystem::directory_iterator{"/"},
                                  std::filesystem::directory_iterator{}),
                    0);
                ASSERT_EQ(std::filesystem::canonical(".."), "/");
              }()),
              testing::ExitedWithCode(0),
              "");
}

void accessFs(const std::filesystem::path &dir, bool should_succeed) {
  // write
  std::ofstream out{"./test"};
  ASSERT_EQ(out.rdstate(), 0) << "Failed to open file: " << strerror(errno);

  out << "#!/bin/bash";
  ASSERT_EQ(out.rdstate(), 0) << "Failed to write file: " << strerror(errno);

  out.close();

  // read
  std::ifstream in{"./test"};
  ASSERT_EQ(in.rdstate(), 0) << "Failed to open file: " << strerror(errno);

  std::string text;
  in >> text;
  ASSERT_EQ(in.rdstate(), std::ios_base::eofbit)
      << "Failed to read file: " << strerror(errno);

  in.close();

  ASSERT_EQ(text, "#!/bin/bash");

  // execute
  std::filesystem::permissions("./test",
                               std::filesystem::perms::owner_exec,
                               std::filesystem::perm_options::add);
  if (should_succeed) {
    ASSERT_EQ(system("./test"), 0);
  } else {
    ASSERT_NE(system("./test"), 0);
  }
  try {
    std::filesystem::create_directories("./test_dir");
    std::filesystem::remove("./test_dir");
    std::filesystem::remove("./test");
  } catch (...) {
    if (should_succeed) {
      throw;
    }
  }
}

TEST(SecureMode, LandlockWorks) {
  auto dir = std::filesystem::temp_directory_path() / "kagome_secure_mode_test";
  std::filesystem::create_directories(dir);
  std::filesystem::current_path(dir);
  accessFs(dir, true);
  ASSERT_OUTCOME_SUCCESS_TRY(enableLandlock(dir));
  accessFs(dir, false);
}
