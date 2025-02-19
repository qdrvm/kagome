/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <algorithm>  // for std::copy
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include "primitives/math.hpp"
#include "pvm/api.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::pvm;

class PvmTest : public testing::Test {};

TEST(PvmTest, test_blog_1) {
  std::vector<uint8_t> program = {
      0x50, 0x56, 0x4d, 0x00, 0x01, 0x01, 0x04, 0x00, 0x00, 0x90, 0x00,
      0x04, 0x15, 0x01, 0x00, 0x00, 0x00, 0x00, 0x67, 0x65, 0x74, 0x5f,
      0x74, 0x68, 0x69, 0x72, 0x64, 0x5f, 0x6e, 0x75, 0x6d, 0x62, 0x65,
      0x72, 0x05, 0x0e, 0x01, 0x00, 0x0b, 0x61, 0x64, 0x64, 0x5f, 0x6e,
      0x75, 0x6d, 0x62, 0x65, 0x72, 0x73, 0x06, 0x20, 0x00, 0x00, 0x19,
      0x02, 0x11, 0xf8, 0x03, 0x10, 0x04, 0x03, 0x15, 0x08, 0x78, 0x05,
      0x4e, 0x08, 0x57, 0x07, 0x01, 0x10, 0x04, 0x01, 0x15, 0x02, 0x11,
      0x08, 0x13, 0x00, 0x49, 0x99, 0x94, 0xfe, 0x00};

  EXPECT_OUTCOME_TRUE(program_blob,
                      ProgramBlob::create_from(std::move(program)));
  ASSERT_EQ(program_blob.memory_config,
            (ProgramBlob::MemoryConfig{
                .ro_data_size = 0,
                .rw_data_size = 0,
                .stack_size = 4096,
            }));

  ASSERT_FALSE(program_blob.ro_data.has_value());
  ASSERT_FALSE(program_blob.rw_data.has_value());

  ASSERT_TRUE(program_blob.code_and_jump_table);
  ASSERT_EQ(program_blob.code_and_jump_table->size(), 32);

  ASSERT_TRUE(program_blob.import_offsets);
  ASSERT_EQ(program_blob.import_offsets->size(), 4);

  ASSERT_TRUE(program_blob.import_symbols);
  ASSERT_EQ(program_blob.import_symbols->size(), 16);

  ASSERT_TRUE(program_blob.exports);
  ASSERT_EQ(program_blob.exports->size(), 14);
}

TEST(PvmTest, doom) {
  std::unique_ptr<FILE, decltype(&std::fclose)> fp(
      std::fopen("/home/iceseer/Work/kagome/test/pvm/doom/doom.polkavm", "r"),
      &std::fclose);

  std::vector<uint8_t> program;
  program.resize(10 * 1024 * 1024);

  std::ignore =
      std::fread(&program[0], sizeof(program[0]), program.size(), fp.get());
  EXPECT_OUTCOME_TRUE(program_blob,
                      ProgramBlob::create_from(std::move(program)));

  EXPECT_OUTCOME_TRUE(config, Config::from_env());
  EXPECT_OUTCOME_TRUE(engine, Engine::create(config));

}
