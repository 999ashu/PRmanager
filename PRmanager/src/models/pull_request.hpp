#pragma once

#include <optional>
#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <vector>

namespace prmanager::models {

struct PullRequest {
  std::string pull_request_id;
  std::string pull_request_name;
  std::string author_id;
  std::string status;
  std::vector<std::string> assigned_reviewers;
  std::optional<std::string> created_at;
  std::optional<std::string> merged_at;
};

struct PullRequestShort {
  std::string pull_request_id;
  std::string pull_request_name;
  std::string author_id;
  std::string status;
};

userver::formats::json::Value Serialize(
    const PullRequest& pr,
    userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value Serialize(
    const PullRequestShort& pr,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace prmanager::models
