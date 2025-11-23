#pragma once

#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <vector>

namespace prmanager::models {

struct TeamMember {
  std::string user_id;
  std::string username;
  bool is_active;
};

struct Team {
  std::string team_name;
  std::vector<TeamMember> members;
};

TeamMember Parse(const userver::formats::json::Value& json,
                 userver::formats::parse::To<TeamMember>);

Team Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<Team>);

userver::formats::json::Value Serialize(
    const Team& team,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace prmanager::models
