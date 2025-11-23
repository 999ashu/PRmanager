#include "stats_handler.hpp"
#include "../models/stats_model.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>

namespace prmanager::handlers {

StatsHandler::StatsHandler(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string StatsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest&,
    userver::server::request::RequestContext&) const {
  auto res_teams =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           "SELECT COUNT(*) FROM prmanager.teams");
  auto res_users =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           "SELECT COUNT(*) FROM prmanager.users");
  auto res_prs =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           "SELECT COUNT(*) FROM prmanager.pull_requests");

  models::StatsResponse response;
  response.teams_count = res_teams[0][0].As<int>();
  response.users_count = res_users[0][0].As<int>();
  response.prs_count = res_prs[0][0].As<int>();

  return userver::formats::json::ToString(models::Serialize(
      response,
      userver::formats::serialize::To<userver::formats::json::Value>{}));
}

}  // namespace prmanager::handlers
