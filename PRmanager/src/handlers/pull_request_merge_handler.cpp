#include "pull_request_merge_handler.hpp"
#include "../models/pull_request_model.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/utils/datetime.hpp>

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

PullRequestMergeHandler::PullRequestMergeHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string PullRequestMergeHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto pr_id = body["pull_request_id"].As<std::string>();

  auto trx = pg_cluster_->Begin(
      "pr_merge", userver::storages::postgres::ClusterHostType::kMaster, {});

  try {
    auto res_pr = trx.Execute(
        "UPDATE prmanager.pull_requests SET status = 'MERGED', merged_at = "
        "COALESCE(merged_at, NOW()) "
        "WHERE id = $1 RETURNING id, name, author_id, status, merged_at",
        pr_id);

    if (res_pr.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return userver::formats::json::ToString(
          BuildErrorResponse("NOT_FOUND", "PR not found"));
    }

    auto res_reviewers = trx.Execute(
        "SELECT reviewer_id FROM prmanager.reviewers WHERE pull_request_id = "
        "$1",
        pr_id);
    std::vector<std::string> reviewers;
    for (const auto& row : res_reviewers) {
      reviewers.push_back(row["reviewer_id"].As<std::string>());
    }

    trx.Commit();

    const auto& row = res_pr[0];
    models::PullRequest pr;
    pr.pull_request_id = row["id"].As<std::string>();
    pr.pull_request_name = row["name"].As<std::string>();
    pr.author_id = row["author_id"].As<std::string>();
    pr.status = row["status"].As<std::string>();
    pr.assigned_reviewers = reviewers;
    pr.merged_at = userver::utils::datetime::Timestring(
        row["merged_at"]
            .As<userver::storages::postgres::TimePointTz>()
            .GetUnderlying());

    userver::formats::json::ValueBuilder response;
    response["pr"] = pr;
    return userver::formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }
}

}  // namespace prmanager::handlers
