#include "user_get_review.hpp"
#include "../models/pull_request.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>

namespace prmanager::handlers {

UserGetReviewHandler::UserGetReviewHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string UserGetReviewHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto& user_id = request.GetArg("user_id");
  if (user_id.empty()) {
    throw userver::server::handlers::ClientError(
        userver::server::handlers::ExternalBody{"Missing user_id"});
  }

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT pr.id, pr.name, pr.author_id, pr.status "
      "FROM prmanager.pull_requests pr "
      "JOIN prmanager.reviewers r ON pr.id = r.pull_request_id "
      "WHERE r.reviewer_id = $1",
      user_id);

  std::vector<models::PullRequestShort> prs;
  for (const auto& row : res) {
    prs.push_back(models::PullRequestShort{
        row["id"].As<std::string>(), row["name"].As<std::string>(),
        row["author_id"].As<std::string>(), row["status"].As<std::string>()});
  }

  userver::formats::json::ValueBuilder response;
  response["user_id"] = user_id;
  response["pull_requests"] = prs;
  return userver::formats::json::ToString(response.ExtractValue());
}

}  // namespace prmanager::handlers
