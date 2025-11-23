#include <string>
#include <vector>
#include <optional>

#include <userver/utest/utest.hpp>
#include <userver/formats/json.hpp>

#include "models/pull_request.hpp"

using prmanager::models::PullRequest;

UTEST(PullRequestSerialize, OptionalFields) {
  PullRequest pr{"pr1",
                 "Fix bug",
                 "author1",
                 "open",
                 std::vector<std::string>{"r1", "r2"},
                 std::optional<std::string>{"2025-11-23T00:00:00Z"},
                 std::optional<std::string>{}};
  auto json = prmanager::models::Serialize(
      pr, userver::formats::serialize::To<userver::formats::json::Value>{});

  EXPECT_EQ(json["pull_request_id"].As<std::string>(), "pr1");
  EXPECT_TRUE(json["createdAt"].IsString());
  EXPECT_TRUE(json["mergedAt"].IsMissing());
}
