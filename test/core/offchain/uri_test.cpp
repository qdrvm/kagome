/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "offchain/impl/uri.hpp"

using kagome::offchain::Uri;

TEST(UriTest, CorrectFullURL) {
  auto original_url =
      "schema://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::Parse(original_url);

  EXPECT_EQ(uri.schema(), "schema");
  EXPECT_EQ(uri.host(), "hostname");
  EXPECT_EQ(uri.port(), "12345");
  EXPECT_EQ(uri.path(), "/path/to/resource");
  EXPECT_EQ(uri.query(), "alpha=A&beta=B");
  EXPECT_EQ(uri.fragment(), "anchor");
  EXPECT_EQ(uri.toString(), original_url);
}

TEST(UriTest, CorrectWithoutSchema) {
  {
    auto original_url =
        "://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::Parse(original_url);

    EXPECT_EQ(uri.schema(), "");
    EXPECT_EQ(uri.host(), "hostname");
    EXPECT_EQ(uri.port(), "12345");
    EXPECT_EQ(uri.path(), "/path/to/resource");
    EXPECT_EQ(uri.query(), "alpha=A&beta=B");
    EXPECT_EQ(uri.fragment(), "anchor");
    EXPECT_EQ(uri.toString(), original_url);
  }
  {
    auto original_url = "hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::Parse(original_url);

    EXPECT_EQ(uri.schema(), "");
    EXPECT_EQ(uri.host(), "hostname");
    EXPECT_EQ(uri.port(), "12345");
    EXPECT_EQ(uri.path(), "/path/to/resource");
    EXPECT_EQ(uri.query(), "alpha=A&beta=B");
    EXPECT_EQ(uri.fragment(), "anchor");
    EXPECT_EQ(uri.toString(), original_url);
  }
}

TEST(UriTest, CorrectWithoutPort) {
  auto original_url =
      "schema://hostname/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::Parse(original_url);

  EXPECT_EQ(uri.schema(), "schema");
  EXPECT_EQ(uri.host(), "hostname");
  EXPECT_EQ(uri.port(), "");
  EXPECT_EQ(uri.path(), "/path/to/resource");
  EXPECT_EQ(uri.query(), "alpha=A&beta=B");
  EXPECT_EQ(uri.fragment(), "anchor");
  EXPECT_EQ(uri.toString(), original_url);
}

TEST(UriTest, CorrectWithoutQuery) {
  auto original_url = "schema://hostname:12345/path/to/resource#anchor";

  auto uri = Uri::Parse(original_url);

  EXPECT_EQ(uri.schema(), "schema");
  EXPECT_EQ(uri.host(), "hostname");
  EXPECT_EQ(uri.port(), "12345");
  EXPECT_EQ(uri.path(), "/path/to/resource");
  EXPECT_EQ(uri.query(), "");
  EXPECT_EQ(uri.fragment(), "anchor");
  EXPECT_EQ(uri.toString(), original_url);
}

TEST(UriTest, CorrectWithoutFragment) {
  auto original_url = "schema://hostname:12345/path/to/resource?alpha=A&beta=B";

  auto uri = Uri::Parse(original_url);

  EXPECT_EQ(uri.schema(), "schema");
  EXPECT_EQ(uri.host(), "hostname");
  EXPECT_EQ(uri.port(), "12345");
  EXPECT_EQ(uri.path(), "/path/to/resource");
  EXPECT_EQ(uri.query(), "alpha=A&beta=B");
  EXPECT_EQ(uri.fragment(), "");
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
    auto original_url = "https://:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
}

TEST(UriTest, CorrectInvalidPort) {
  auto error = "Invalid port";
  {
    auto original_url = "https://google.com:Azaza/";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
  {
    auto original_url = "https://google.com:77777/";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
  {
    auto original_url = "https://google.com:/";

    auto uri = Uri::Parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
}

TEST(UriTest, Copy) {
  auto original_url =
      "schema://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  {
    auto uri1 = Uri::Parse(original_url);
    Uri uri2(uri1);

    EXPECT_EQ(uri1.schema(), "schema");
    EXPECT_EQ(uri1.host(), "hostname");
    EXPECT_EQ(uri1.port(), "12345");
    EXPECT_EQ(uri1.path(), "/path/to/resource");
    EXPECT_EQ(uri1.query(), "alpha=A&beta=B");
    EXPECT_EQ(uri1.fragment(), "anchor");
    EXPECT_EQ(uri1.toString(), original_url);

    EXPECT_EQ(uri2.schema(), "schema");
    EXPECT_EQ(uri2.host(), "hostname");
    EXPECT_EQ(uri2.port(), "12345");
    EXPECT_EQ(uri2.path(), "/path/to/resource");
    EXPECT_EQ(uri2.query(), "alpha=A&beta=B");
    EXPECT_EQ(uri2.fragment(), "anchor");
    EXPECT_EQ(uri2.toString(), original_url);
  }
  {
    auto uri1 = Uri::Parse(original_url);
    Uri uri2;
    uri2 = uri1;

    EXPECT_EQ(uri1.schema(), "schema");
    EXPECT_EQ(uri1.host(), "hostname");
    EXPECT_EQ(uri1.port(), "12345");
    EXPECT_EQ(uri1.path(), "/path/to/resource");
    EXPECT_EQ(uri1.query(), "alpha=A&beta=B");
    EXPECT_EQ(uri1.fragment(), "anchor");
    EXPECT_EQ(uri1.toString(), original_url);

    EXPECT_EQ(uri2.schema(), "schema");
    EXPECT_EQ(uri2.host(), "hostname");
    EXPECT_EQ(uri2.port(), "12345");
    EXPECT_EQ(uri2.path(), "/path/to/resource");
    EXPECT_EQ(uri2.query(), "alpha=A&beta=B");
    EXPECT_EQ(uri2.fragment(), "anchor");
    EXPECT_EQ(uri2.toString(), original_url);
  }
}

TEST(UriTest, Move) {
  auto original_url =
      "schema://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  {
    auto uri1 = Uri::Parse(original_url);
    Uri uri2(std::move(uri1));

    EXPECT_EQ(uri1.schema(), "");
    EXPECT_EQ(uri1.host(), "");
    EXPECT_EQ(uri1.port(), "");
    EXPECT_EQ(uri1.path(), "");
    EXPECT_EQ(uri1.query(), "");
    EXPECT_EQ(uri1.fragment(), "");
    EXPECT_EQ(uri1.toString(), "");

    EXPECT_EQ(uri2.schema(), "schema");
    EXPECT_EQ(uri2.host(), "hostname");
    EXPECT_EQ(uri2.port(), "12345");
    EXPECT_EQ(uri2.path(), "/path/to/resource");
    EXPECT_EQ(uri2.query(), "alpha=A&beta=B");
    EXPECT_EQ(uri2.fragment(), "anchor");
    EXPECT_EQ(uri2.toString(), original_url);
  }
  {
    auto uri1 = Uri::Parse(original_url);
    Uri uri2;
    uri2 = std::move(uri1);

    EXPECT_EQ(uri1.schema(), "");
    EXPECT_EQ(uri1.host(), "");
    EXPECT_EQ(uri1.port(), "");
    EXPECT_EQ(uri1.path(), "");
    EXPECT_EQ(uri1.query(), "");
    EXPECT_EQ(uri1.fragment(), "");
    EXPECT_EQ(uri1.toString(), "");

    EXPECT_EQ(uri2.schema(), "schema");
    EXPECT_EQ(uri2.host(), "hostname");
    EXPECT_EQ(uri2.port(), "12345");
    EXPECT_EQ(uri2.path(), "/path/to/resource");
    EXPECT_EQ(uri2.query(), "alpha=A&beta=B");
    EXPECT_EQ(uri2.fragment(), "anchor");
    EXPECT_EQ(uri2.toString(), original_url);
  }
}
