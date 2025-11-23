#include "team_add.hpp"
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

TeamAddHandler::TeamAddHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string TeamAddHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto team = body.As<models::Team>();

  auto trx = pg_cluster_->Begin(
      "team_add", userver::storages::postgres::ClusterHostType::kMaster, {});

  try {
    auto res = trx.Execute("SELECT 1 FROM prmanager.teams WHERE name = $1",
                           team.team_name);
    if (!res.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::ToString(
          BuildErrorResponse("TEAM_EXISTS", "team_name already exists"));
    }

    trx.Execute("INSERT INTO prmanager.teams (name) VALUES ($1)",
                team.team_name);

    for (const auto& member : team.members) {
      trx.Execute(
          "INSERT INTO prmanager.users (id, username, team_name, is_active) "
          "VALUES ($1, $2, $3, $4) "
          "ON CONFLICT (id) DO UPDATE SET username = $2, team_name = $3, "
          "is_active = $4",
          member.user_id, member.username, team.team_name, member.is_active);
    }

    trx.Commit();
  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
  userver::formats::json::ValueBuilder response;
  response["team"] = team;
  return userver::formats::json::ToString(response.ExtractValue());
}

}  // namespace prmanager::handlers
