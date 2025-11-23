#pragma once

#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <vector>

namespace prmanager::models {

struct User {
  std::string user_id;
  std::string username;
  std::string team_name;
  bool is_active;
};

struct MassDeactivateRequest {
  std::vector<std::string> user_ids;
};

userver::formats::json::Value Serialize(
    const User& user,
    userver::formats::serialize::To<userver::formats::json::Value>);

MassDeactivateRequest Parse(const userver::formats::json::Value& json,
                            userver::formats::parse::To<MassDeactivateRequest>);

}  // namespace prmanager::models
