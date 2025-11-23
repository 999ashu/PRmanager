#include "pull_request_create_handler.hpp"
#include "../models/pull_request_model.hpp"

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

PullRequestCreateHandler::PullRequestCreateHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string PullRequestCreateHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto pr_id = body["pull_request_id"].As<std::string>();
  const auto pr_name = body["pull_request_name"].As<std::string>();
  const auto author_id = body["author_id"].As<std::string>();

  auto trx = pg_cluster_->Begin(
      "pr_create", userver::storages::postgres::ClusterHostType::kMaster, {});

  try {
    auto res_pr = trx.Execute(
        "SELECT 1 FROM prmanager.pull_requests WHERE id = $1", pr_id);
    if (!res_pr.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::ToString(
          BuildErrorResponse("PR_EXISTS", "PR id already exists"));
    }

    auto res_author = trx.Execute(
        "SELECT team_name FROM prmanager.users WHERE id = $1", author_id);
    if (res_author.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return userver::formats::json::ToString(
          BuildErrorResponse("NOT_FOUND", "Author not found"));
    }
    const auto team_name = res_author[0]["team_name"].As<std::string>();

    auto res_candidates = trx.Execute(
        "SELECT id FROM prmanager.users WHERE team_name = $1 AND is_active = "
        "TRUE AND id != $2",
        team_name, author_id);

    std::vector<std::string> candidates;
    for (const auto& row : res_candidates) {
      candidates.push_back(row["id"].As<std::string>());
    }

    std::vector<std::string> reviewers;
    if (candidates.size() <= 2) {
      reviewers = candidates;
    } else {
      std::sample(candidates.begin(), candidates.end(),
                  std::back_inserter(reviewers), 2,
                  std::mt19937{std::random_device{}()});
    }

    trx.Execute(
        "INSERT INTO prmanager.pull_requests (id, name, author_id, status) "
        "VALUES ($1, $2, $3, 'OPEN')",
        pr_id, pr_name, author_id);

    for (const auto& reviewer : reviewers) {
      trx.Execute(
          "INSERT INTO prmanager.reviewers (pull_request_id, reviewer_id) "
          "VALUES ($1, $2)",
          pr_id, reviewer);
    }

    trx.Commit();

    models::PullRequest pr;
    pr.pull_request_id = pr_id;
    pr.pull_request_name = pr_name;
    pr.author_id = author_id;
    pr.status = "OPEN";
    pr.assigned_reviewers = reviewers;

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    userver::formats::json::ValueBuilder response;
    response["pr"] = pr;
    return userver::formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }
}

}  // namespace prmanager::handlers
