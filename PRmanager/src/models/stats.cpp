#include "stats.hpp"

namespace prmanager::models {

userver::formats::json::Value Serialize(
    const MassDeactivateResponse& response,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["deactivated_count"] = response.deactivated_count;
  return builder.ExtractValue();
}

userver::formats::json::Value Serialize(
    const StatsResponse& response,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["teams_count"] = response.teams_count;
  builder["users_count"] = response.users_count;
  builder["prs_count"] = response.prs_count;
  return builder.ExtractValue();
}

}  // namespace prmanager::models
