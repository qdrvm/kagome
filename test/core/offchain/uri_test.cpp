/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "offchain/impl/uri.hpp"

using kagome::offchain::Uri;

TEST(UriTest, CorrectFullURL) {
  auto original_url =
      "schema://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::Parse(original_url);

  EXPECT_EQ(uri.Schema, "schema");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "12345");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "alpha=A&beta=B");
  EXPECT_EQ(uri.Fragment, "anchor");
  EXPECT_EQ(uri.toString(), original_url);
}

TEST(UriTest, CorrectWithoutSchema) {
  {
    auto original_url =
        "://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::Parse(original_url);

    EXPECT_EQ(uri.Schema, "");
    EXPECT_EQ(uri.Host, "hostname");
    EXPECT_EQ(uri.Port, "12345");
    EXPECT_EQ(uri.Path, "/path/to/resource");
    EXPECT_EQ(uri.Query, "alpha=A&beta=B");
    EXPECT_EQ(uri.Fragment, "anchor");
    EXPECT_EQ(uri.toString(), original_url);
  }
  {
    auto original_url =
        "hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::Parse(original_url);

    EXPECT_EQ(uri.Schema, "");
    EXPECT_EQ(uri.Host, "hostname");
    EXPECT_EQ(uri.Port, "12345");
    EXPECT_EQ(uri.Path, "/path/to/resource");
    EXPECT_EQ(uri.Query, "alpha=A&beta=B");
    EXPECT_EQ(uri.Fragment, "anchor");
    EXPECT_EQ(uri.toString(), original_url);
  }
}

TEST(UriTest, CorrectWithoutPort) {
  auto original_url =
      "schema://hostname/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::Parse(original_url);

  EXPECT_EQ(uri.Schema, "schema");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "alpha=A&beta=B");
  EXPECT_EQ(uri.Fragment, "anchor");
  EXPECT_EQ(uri.toString(), original_url);
}

TEST(UriTest, CorrectWithoutQuery) {
  auto original_url =
      "schema://hostname:12345/path/to/resource#anchor";

  auto uri = Uri::Parse(original_url);

  EXPECT_EQ(uri.Schema, "schema");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "12345");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "");
  EXPECT_EQ(uri.Fragment, "anchor");
  EXPECT_EQ(uri.toString(), original_url);
}

TEST(UriTest, CorrectWithoutFragment) {
  auto original_url =
      "schema://hostname:12345/path/to/resource?alpha=A&beta=B";

  auto uri = Uri::Parse(original_url);

  EXPECT_EQ(uri.Schema, "schema");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "12345");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "alpha=A&beta=B");
  EXPECT_EQ(uri.Fragment, "");
  EXPECT_EQ(uri.toString(), original_url);
}

TEST(UriTest, CorrectInvalidSchema) {
  auto error = "Invalid schema";

  auto original_url =
      "~azaza~://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::Parse(original_url);

  ASSERT_TRUE(uri.error().has_value());
  EXPECT_EQ(uri.error().value(), error);
}

TEST(UriTest, CorrectInvalidHostname) {
  auto error = "Invalid hostname";
  {
    auto original_url =
        "https://goggle,com:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
  {
    auto original_url =
        "https://:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
}

TEST(UriTest, CorrectInvalidPort) {
  auto error = "Invalid port";
  {
    auto original_url =
        "https://google.com:Azaza/";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
  {
    auto original_url =
        "https://google.com:77777/";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
  {
    auto original_url =
        "https://google.com:/";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
}
