/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

#include "application/impl/app_state_manager_impl.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::application::AppStateException;
using kagome::application::AppStateManager;
using kagome::application::AppStateManagerImpl;
using OnPrepare = kagome::application::AppStateManager::OnPrepare;
using OnLaunch = kagome::application::AppStateManager::OnLaunch;
using OnShutdown = kagome::application::AppStateManager::OnShutdown;

using testing::Return;
using testing::Sequence;

class OnPrepareMock {
 public:
  MOCK_METHOD(bool, call, ());
  bool operator()() {
    return call();
  }
};
class OnLaunchMock {
 public:
  MOCK_METHOD(bool, call, ());
  bool operator()() {
    return call();
  }
};
class OnShutdownMock {
 public:
  MOCK_METHOD(void, call, ());
  void operator()() {
    return call();
  }
};

class AppStateManagerTest : public AppStateManagerImpl, public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    reset();
    prepare_cb = std::make_shared<OnPrepareMock>();
    launch_cb = std::make_shared<OnLaunchMock>();
    shutdown_cb = std::make_shared<OnShutdownMock>();
  }
  void TearDown() override {
    prepare_cb.reset();
    launch_cb.reset();
    shutdown_cb.reset();
  }

  std::shared_ptr<OnPrepareMock> prepare_cb;
  std::shared_ptr<OnLaunchMock> launch_cb;
  std::shared_ptr<OnShutdownMock> shutdown_cb;
};

/**
 * @given new created AppStateManager
 * @when switch stages in order
 * @then state changes according to the order
 */
TEST_F(AppStateManagerTest, StateSequence_Normal) {
  ASSERT_EQ(state(), AppStateManager::State::Init);
  ASSERT_NO_THROW(doPrepare());
  ASSERT_EQ(state(), AppStateManager::State::ReadyToStart);
  ASSERT_NO_THROW(doLaunch());
  ASSERT_EQ(state(), AppStateManager::State::Works);
  ASSERT_NO_THROW(doShutdown());
  ASSERT_EQ(state(), AppStateManager::State::ReadyToStop);
}

/**
 * @given AppStateManager in state 'ReadyToStart' (after stage 'prepare')
 * @when trying to run stage 'prepare' again
 * @then thrown exception, state wasn't change. Can to run stages 'launch' and
 * 'shutdown'
 */
TEST_F(AppStateManagerTest, StateSequence_Abnormal_1) {
  doPrepare();
  EXPECT_THROW(doPrepare(), AppStateException);
  EXPECT_EQ(state(), AppStateManager::State::ReadyToStart);
  EXPECT_NO_THROW(doLaunch());
  EXPECT_NO_THROW(doShutdown());
}

/**
 * @given AppStateManager in state 'Works' (after stage 'launch')
 * @when trying to run stages 'prepare' and 'launch' again
 * @then thrown exceptions, state wasn't change. Can to run stages 'launch' and
 * 'shutdown'
 */
TEST_F(AppStateManagerTest, StateSequence_Abnormal_2) {
  doPrepare();
  doLaunch();
  EXPECT_THROW(doPrepare(), AppStateException);
  EXPECT_THROW(doLaunch(), AppStateException);
  EXPECT_EQ(state(), AppStateManager::State::Works);
  EXPECT_NO_THROW(doShutdown());
}

/**
 * @given AppStateManager in state 'ReadyToStop' (after stage 'shutdown')
 * @when trying to run any stages 'prepare' and 'launch' again
 * @then thrown exceptions, except shutdown. State wasn't change.
 */
TEST_F(AppStateManagerTest, StateSequence_Abnormal_3) {
  doPrepare();
  doLaunch();
  doShutdown();
  EXPECT_THROW(doPrepare(), AppStateException);
  EXPECT_THROW(doLaunch(), AppStateException);
  EXPECT_THROW(doShutdown(), AppStateException);
  EXPECT_EQ(state(), AppStateManager::State::ReadyToStop);
}

/**
 * @given new created AppStateManager
 * @when add callbacks for each stages
 * @then done without exceptions
 */
TEST_F(AppStateManagerTest, AddCallback_Initial) {
  EXPECT_NO_THROW(atPrepare([] {}));
  EXPECT_NO_THROW(atLaunch([] {}));
  EXPECT_NO_THROW(atShutdown([] {}));
}

/**
 * @given AppStateManager in state 'ReadyToStart' (after stage 'prepare')
 * @when add callbacks for each stages
 * @then thrown exception only for 'prepare' stage callback
 */
TEST_F(AppStateManagerTest, AddCallback_AfterPrepare) {
  doPrepare();
  EXPECT_THROW(atPrepare([] {}), AppStateException);
  EXPECT_NO_THROW(atLaunch([] {}));
  EXPECT_NO_THROW(atShutdown([] {}));
}

/**
 * @given AppStateManager in state 'Works' (after stage 'launch')
 * @when add callbacks for each stages
 * @then done without exception only for 'shutdown' stage callback
 */
TEST_F(AppStateManagerTest, AddCallback_AfterLaunch) {
  doPrepare();
  doLaunch();
  EXPECT_THROW(atPrepare([] {}), AppStateException);
  EXPECT_THROW(atLaunch([] {}), AppStateException);
  EXPECT_NO_THROW(atShutdown([] {}));
}

/**
 * @given AppStateManager in state 'ReadyToStop' (after stage 'shutdown')
 * @when add callbacks for each stages
 * @then trown exceptions for each calls
 */
TEST_F(AppStateManagerTest, AddCallback_AfterShutdown) {
  doPrepare();
  doLaunch();
  doShutdown();
  EXPECT_THROW(atPrepare([] {}), AppStateException);
  EXPECT_THROW(atLaunch([] {}), AppStateException);
  EXPECT_THROW(atShutdown([] {}), AppStateException);
}

struct UnderControlObject {
  UnderControlObject(OnPrepareMock &p, OnLaunchMock &l, OnShutdownMock &s)
      : p(p), l(l), s(s) {}

  OnPrepareMock &p;
  OnLaunchMock &l;
  OnShutdownMock &s;
  int tag = 0;

  bool prepare() {
    tag = 1;
    return p();
  }

  bool start() {
    tag = 2;
    return l();
  }

  void stop() {
    tag = 3;
    s();
  }
};

/**
 * @given new created AppStateManager
 * @when register callbacks by reg() method
 * @then each callcack registered for appropriate state
 */
TEST_F(AppStateManagerTest, RegCallbacks) {
  UnderControlObject x(*prepare_cb, *launch_cb, *shutdown_cb);

  takeControl(x);

  EXPECT_CALL(*prepare_cb, call()).WillOnce(Return(true));
  EXPECT_CALL(*launch_cb, call()).WillOnce(Return(true));
  EXPECT_CALL(*shutdown_cb, call()).WillOnce(Return());

  EXPECT_NO_THROW(doPrepare());
  EXPECT_EQ(x.tag, 1);
  EXPECT_NO_THROW(doLaunch());
  EXPECT_EQ(x.tag, 2);
  EXPECT_NO_THROW(doShutdown());
  EXPECT_EQ(x.tag, 3);
}

/**
 * @given new created AppStateManager
 * @when register callbacks by reg() method and run() AppStateManager
 * @then each callcack execudted according to the stages order
 */
TEST_F(AppStateManagerTest, Run_CallSequence) {
  EXPECT_THROW(run(), std::logic_error);

  auto app_state_manager = std::make_shared<AppStateManagerImpl>();

  UnderControlObject x(*prepare_cb, *launch_cb, *shutdown_cb);

  app_state_manager->takeControl(x);

  Sequence seq;
  EXPECT_CALL(*prepare_cb, call()).InSequence(seq).WillOnce(Return(true));
  EXPECT_CALL(*launch_cb, call()).InSequence(seq).WillOnce(Return(true));
  EXPECT_CALL(*shutdown_cb, call()).InSequence(seq).WillOnce(Return());

  app_state_manager->atLaunch([] {
    std::thread terminator([] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      raise(SIGQUIT);
    });
    terminator.join();
    return true;
  });

  std::thread main([&] { EXPECT_NO_THROW(app_state_manager->run()); });
  main.join();
}
