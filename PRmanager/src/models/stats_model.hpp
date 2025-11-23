#pragma once

#include <userver/formats/json.hpp>

namespace prmanager::models {

struct MassDeactivateResponse {
  int deactivated_count;
};

struct StatsResponse {
  int teams_count;
  int users_count;
  int prs_count;
};

userver::formats::json::Value Serialize(
    const MassDeactivateResponse& response,
    userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value Serialize(
    const StatsResponse& response,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace prmanager::models
