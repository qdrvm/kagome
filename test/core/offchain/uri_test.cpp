/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/uri.hpp"

using kagome::common::Uri;

TEST(UriTest, CorrectFullURL) {
  auto original_url =
      "schema://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::parse(original_url);

  EXPECT_EQ(uri.Schema, "schema");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "12345");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "alpha=A&beta=B");
  EXPECT_EQ(uri.Fragment, "anchor");
  EXPECT_EQ(uri.to_string(), original_url);
}

TEST(UriTest, CorrectWithoutSchema) {
  auto original_url = "hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::parse(original_url);

  EXPECT_EQ(uri.Schema, "");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "12345");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "alpha=A&beta=B");
  EXPECT_EQ(uri.Fragment, "anchor");
  EXPECT_EQ(uri.to_string(), original_url);
}

TEST(UriTest, CorrectWithoutPort) {
  auto original_url =
      "schema://hostname/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::parse(original_url);

  EXPECT_EQ(uri.Schema, "schema");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "alpha=A&beta=B");
  EXPECT_EQ(uri.Fragment, "anchor");
  EXPECT_EQ(uri.to_string(), original_url);
}

TEST(UriTest, CorrectWithoutQuery) {
  auto original_url = "schema://hostname:12345/path/to/resource#anchor";

  auto uri = Uri::parse(original_url);

  EXPECT_EQ(uri.Schema, "schema");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "12345");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "");
  EXPECT_EQ(uri.Fragment, "anchor");
  EXPECT_EQ(uri.to_string(), original_url);
}

TEST(UriTest, CorrectWithoutFragment) {
  auto original_url = "schema://hostname:12345/path/to/resource?alpha=A&beta=B";

  auto uri = Uri::parse(original_url);

  EXPECT_EQ(uri.Schema, "schema");
  EXPECT_EQ(uri.Host, "hostname");
  EXPECT_EQ(uri.Port, "12345");
  EXPECT_EQ(uri.Path, "/path/to/resource");
  EXPECT_EQ(uri.Query, "alpha=A&beta=B");
  EXPECT_EQ(uri.Fragment, "");
  EXPECT_EQ(uri.to_string(), original_url);
}

TEST(UriTest, CorrectInvalidSchema) {
  auto error = "Invalid schema";

  auto original_url =
      "~azaza~://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  auto uri = Uri::parse(original_url);

  ASSERT_TRUE(uri.error().has_value());
  EXPECT_EQ(uri.error().value(), error);
}

TEST(UriTest, CorrectInvalidHostname) {
  auto error = "Invalid hostname";
  {
    auto original_url =
        "https://goggle,com:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
  {
    auto original_url = "https://:12345/path/to/resource?alpha=A&beta=B#anchor";

    auto uri = Uri::parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
}

TEST(UriTest, CorrectInvalidPort) {
  auto error = "Invalid port";
  {
    auto original_url = "https://google.com:Azaza/";

    auto uri = Uri::parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
  {
    auto original_url = "https://google.com:77777/";

    auto uri = Uri::parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
  {
    auto original_url = "https://google.com:/";

    auto uri = Uri::parse(original_url);

    ASSERT_TRUE(uri.error().has_value());
    EXPECT_EQ(uri.error().value(), error);
  }
}

TEST(UriTest, Copy) {
  auto original_url =
      "schema://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  {
    auto uri1 = Uri::parse(original_url);
    Uri uri2(uri1);

    EXPECT_EQ(uri1.Schema, "schema");
    EXPECT_EQ(uri1.Host, "hostname");
    EXPECT_EQ(uri1.Port, "12345");
    EXPECT_EQ(uri1.Path, "/path/to/resource");
    EXPECT_EQ(uri1.Query, "alpha=A&beta=B");
    EXPECT_EQ(uri1.Fragment, "anchor");
    EXPECT_EQ(uri1.to_string(), original_url);

    EXPECT_EQ(uri2.Schema, "schema");
    EXPECT_EQ(uri2.Host, "hostname");
    EXPECT_EQ(uri2.Port, "12345");
    EXPECT_EQ(uri2.Path, "/path/to/resource");
    EXPECT_EQ(uri2.Query, "alpha=A&beta=B");
    EXPECT_EQ(uri2.Fragment, "anchor");
    EXPECT_EQ(uri2.to_string(), original_url);
  }
  {
    auto uri1 = Uri::parse(original_url);
    Uri uri2;
    uri2 = uri1;

    EXPECT_EQ(uri1.Schema, "schema");
    EXPECT_EQ(uri1.Host, "hostname");
    EXPECT_EQ(uri1.Port, "12345");
    EXPECT_EQ(uri1.Path, "/path/to/resource");
    EXPECT_EQ(uri1.Query, "alpha=A&beta=B");
    EXPECT_EQ(uri1.Fragment, "anchor");
    EXPECT_EQ(uri1.to_string(), original_url);

    EXPECT_EQ(uri2.Schema, "schema");
    EXPECT_EQ(uri2.Host, "hostname");
    EXPECT_EQ(uri2.Port, "12345");
    EXPECT_EQ(uri2.Path, "/path/to/resource");
    EXPECT_EQ(uri2.Query, "alpha=A&beta=B");
    EXPECT_EQ(uri2.Fragment, "anchor");
    EXPECT_EQ(uri2.to_string(), original_url);
  }
}

TEST(UriTest, Move) {
  auto original_url =
      "schema://hostname:12345/path/to/resource?alpha=A&beta=B#anchor";

  {
    auto uri1 = Uri::parse(original_url);
    Uri uri2(std::move(uri1));

    EXPECT_EQ(uri1.Schema, "");
    EXPECT_EQ(uri1.Host, "");
    EXPECT_EQ(uri1.Port, "");
    EXPECT_EQ(uri1.Path, "");
    EXPECT_EQ(uri1.Query, "");
    EXPECT_EQ(uri1.Fragment, "");
    EXPECT_EQ(uri1.to_string(), "");

    EXPECT_EQ(uri2.Schema, "schema");
    EXPECT_EQ(uri2.Host, "hostname");
    EXPECT_EQ(uri2.Port, "12345");
    EXPECT_EQ(uri2.Path, "/path/to/resource");
    EXPECT_EQ(uri2.Query, "alpha=A&beta=B");
    EXPECT_EQ(uri2.Fragment, "anchor");
    EXPECT_EQ(uri2.to_string(), original_url);
  }
  {
    auto uri1 = Uri::parse(original_url);
    Uri uri2;
    uri2 = std::move(uri1);

    EXPECT_EQ(uri1.Schema, "");
    EXPECT_EQ(uri1.Host, "");
    EXPECT_EQ(uri1.Port, "");
    EXPECT_EQ(uri1.Path, "");
    EXPECT_EQ(uri1.Query, "");
    EXPECT_EQ(uri1.Fragment, "");
    EXPECT_EQ(uri1.to_string(), "");

    EXPECT_EQ(uri2.Schema, "schema");
    EXPECT_EQ(uri2.Host, "hostname");
    EXPECT_EQ(uri2.Port, "12345");
    EXPECT_EQ(uri2.Path, "/path/to/resource");
    EXPECT_EQ(uri2.Query, "alpha=A&beta=B");
    EXPECT_EQ(uri2.Fragment, "anchor");
    EXPECT_EQ(uri2.to_string(), original_url);
  }
}
