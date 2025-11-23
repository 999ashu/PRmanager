#include "team_get.hpp"
#include "../models/team.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>

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

TeamGetHandler::TeamGetHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string TeamGetHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto& team_name = request.GetArg("team_name");
  if (team_name.empty()) {
    throw userver::server::handlers::ClientError(
        userver::server::handlers::ExternalBody{"Missing team_name"});
  }

  auto res_team = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT name FROM prmanager.teams WHERE name = $1", team_name);
  if (res_team.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return userver::formats::json::ToString(
        BuildErrorResponse("NOT_FOUND", "Team not found"));
  }

  auto res_users =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           "SELECT id, username, is_active FROM "
                           "prmanager.users WHERE team_name = $1",
                           team_name);

  models::Team team;
  team.team_name = team_name;
  for (const auto& row : res_users) {
    team.members.push_back(models::TeamMember{row["id"].As<std::string>(),
                                              row["username"].As<std::string>(),
                                              row["is_active"].As<bool>()});
  }

  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder(team).ExtractValue());
}

}  // namespace prmanager::handlers
