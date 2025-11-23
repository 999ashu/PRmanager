#include "team.hpp"

namespace prmanager::models {

TeamMember Parse(const userver::formats::json::Value& json,
                 userver::formats::parse::To<TeamMember>) {
  return TeamMember{json["user_id"].As<std::string>(),
                    json["username"].As<std::string>(),
                    json["is_active"].As<bool>()};
}

Team Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<Team>) {
  return Team{json["team_name"].As<std::string>(),
              json["members"].As<std::vector<TeamMember>>()};
}

userver::formats::json::Value Serialize(
    const TeamMember& member,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["user_id"] = member.user_id;
  builder["username"] = member.username;
  builder["is_active"] = member.is_active;
  return builder.ExtractValue();
}

userver::formats::json::Value Serialize(
    const Team& team,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["team_name"] = team.team_name;
  builder["members"] = team.members;
  return builder.ExtractValue();
}

}  // namespace prmanager::models
