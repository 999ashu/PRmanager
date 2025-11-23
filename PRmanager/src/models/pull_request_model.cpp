#include "pull_request_model.hpp"

namespace prmanager::models {

userver::formats::json::Value Serialize(
    const PullRequest& pr,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["pull_request_id"] = pr.pull_request_id;
  builder["pull_request_name"] = pr.pull_request_name;
  builder["author_id"] = pr.author_id;
  builder["status"] = pr.status;
  builder["assigned_reviewers"] = pr.assigned_reviewers;
  if (pr.created_at) builder["createdAt"] = *pr.created_at;
  if (pr.merged_at) builder["mergedAt"] = *pr.merged_at;
  return builder.ExtractValue();
}

userver::formats::json::Value Serialize(
    const PullRequestShort& pr,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["pull_request_id"] = pr.pull_request_id;
  builder["pull_request_name"] = pr.pull_request_name;
  builder["author_id"] = pr.author_id;
  builder["status"] = pr.status;
  return builder.ExtractValue();
}

}  // namespace prmanager::models
