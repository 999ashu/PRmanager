#include "pull_request_reassign.hpp"
#include "../models/pull_request.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>

#include <algorithm>
#include <random>

namespace prmanager::handlers {

namespace {

userver::formats::json::Value BuildErrorResponse(const std::string& code,
                                                 const std::string& message) {
  userver::formats::json::ValueBuilder builder;
  userver::formats::json::ValueBuilder error_builder;
  error_builder["code"] = code;
  error_builder["message"] = message;
  builder["error"] = error_builder.ExtractValue();
  return builder.ExtractValue();
}

}  // namespace

PullRequestReassignHandler::PullRequestReassignHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string PullRequestReassignHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto pr_id = body["pull_request_id"].As<std::string>();
  const auto old_user_id = body["old_user_id"].As<std::string>();

  auto trx = pg_cluster_->Begin(
      "pr_reassign", userver::storages::postgres::ClusterHostType::kMaster, {});

  try {
    auto res_pr = trx.Execute(
        "SELECT status, author_id FROM prmanager.pull_requests WHERE id = $1",
        pr_id);
    if (res_pr.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return userver::formats::json::ToString(
          BuildErrorResponse("NOT_FOUND", "PR not found"));
    }
    if (res_pr[0]["status"].As<std::string>() == "MERGED") {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::ToString(
          BuildErrorResponse("PR_MERGED", "cannot reassign on merged PR"));
    }
    const auto author_id = res_pr[0]["author_id"].As<std::string>();

    auto res_reviewer = trx.Execute(
        "SELECT 1 FROM prmanager.reviewers WHERE pull_request_id = $1 AND "
        "reviewer_id = $2",
        pr_id, old_user_id);
    if (res_reviewer.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::ToString(BuildErrorResponse(
          "NOT_ASSIGNED", "reviewer is not assigned to this PR"));
    }

    auto res_user = trx.Execute(
        "SELECT team_name FROM prmanager.users WHERE id = $1", old_user_id);
    if (res_user.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return userver::formats::json::ToString(
          BuildErrorResponse("NOT_FOUND", "User not found"));
    }
    const auto team_name = res_user[0]["team_name"].As<std::string>();

    auto res_current_reviewers = trx.Execute(
        "SELECT reviewer_id FROM prmanager.reviewers WHERE pull_request_id = "
        "$1",
        pr_id);
    std::vector<std::string> current_reviewers;
    for (const auto& row : res_current_reviewers) {
      current_reviewers.push_back(row["reviewer_id"].As<std::string>());
    }

    auto res_candidates = trx.Execute(
        "SELECT id FROM prmanager.users WHERE team_name = $1 AND is_active = "
        "TRUE AND id != $2",
        team_name, author_id);

    std::vector<std::string> candidates;
    for (const auto& row : res_candidates) {
      std::string uid = row["id"].As<std::string>();
      bool is_reviewer = false;
      for (const auto& r : current_reviewers) {
        if (r == uid) {
          is_reviewer = true;
          break;
        }
      }
      if (!is_reviewer) {
        candidates.push_back(uid);
      }
    }

    if (candidates.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::ToString(BuildErrorResponse(
          "NO_CANDIDATE", "no active replacement candidate in team"));
    }

    std::string new_reviewer_id;
    std::sample(candidates.begin(), candidates.end(), &new_reviewer_id, 1,
                std::mt19937{std::random_device{}()});

    trx.Execute(
        "DELETE FROM prmanager.reviewers WHERE pull_request_id = $1 AND "
        "reviewer_id = $2",
        pr_id, old_user_id);
    trx.Execute(
        "INSERT INTO prmanager.reviewers (pull_request_id, reviewer_id) VALUES "
        "($1, $2)",
        pr_id, new_reviewer_id);

    trx.Commit();

    std::replace(current_reviewers.begin(), current_reviewers.end(),
                 old_user_id, new_reviewer_id);

    models::PullRequest pr;
    pr.pull_request_id = pr_id;
    auto res_pr_details = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        "SELECT name FROM prmanager.pull_requests WHERE id = $1", pr_id);
    pr.pull_request_name = res_pr_details[0]["name"].As<std::string>();
    pr.author_id = author_id;
    pr.status = "OPEN";
    pr.assigned_reviewers = current_reviewers;

    userver::formats::json::ValueBuilder response;
    response["pr"] = pr;
    response["replaced_by"] = new_reviewer_id;
    return userver::formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }
}

}  // namespace prmanager::handlers
