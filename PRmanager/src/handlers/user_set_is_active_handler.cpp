#include "user_set_is_active_handler.hpp"
#include "../models/user_model.hpp"

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

UserSetIsActiveHandler::UserSetIsActiveHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string UserSetIsActiveHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto user_id = body["user_id"].As<std::string>();
  const auto is_active = body["is_active"].As<bool>();

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "UPDATE prmanager.users SET is_active = $1 WHERE id = $2 "
      "RETURNING id, username, team_name, is_active",
      is_active, user_id);

  if (res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return userver::formats::json::ToString(
        BuildErrorResponse("NOT_FOUND", "User not found"));
  }

  const auto& row = res[0];
  models::User user{
      row["id"].As<std::string>(), row["username"].As<std::string>(),
      row["team_name"].As<std::string>(), row["is_active"].As<bool>()};

  userver::formats::json::ValueBuilder response;
  response["user"] = user;
  return userver::formats::json::ToString(response.ExtractValue());
}

}  // namespace prmanager::handlers
