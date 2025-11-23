#include "user.hpp"

namespace prmanager::models {

userver::formats::json::Value Serialize(
    const User& user,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["user_id"] = user.user_id;
  builder["username"] = user.username;
  builder["team_name"] = user.team_name;
  builder["is_active"] = user.is_active;
  return builder.ExtractValue();
}

MassDeactivateRequest Parse(
    const userver::formats::json::Value& json,
    userver::formats::parse::To<MassDeactivateRequest>) {
  return MassDeactivateRequest{json["user_ids"].As<std::vector<std::string>>()};
}

}  // namespace prmanager::models
