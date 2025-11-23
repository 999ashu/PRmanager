#include <string>
#include <vector>

#include <userver/utest/utest.hpp>
#include <userver/formats/json.hpp>

#include "models/user.hpp"

using prmanager::models::User;
using prmanager::models::MassDeactivateRequest;

UTEST(SerializeUser, Basic) {
  User u{"id1", "alice", "teamA", true};
  auto json = prmanager::models::Serialize(
      u, userver::formats::serialize::To<userver::formats::json::Value>{});
  EXPECT_EQ(json["user_id"].As<std::string>(), "id1");
  EXPECT_EQ(json["username"].As<std::string>(), "alice");
  EXPECT_EQ(json["team_name"].As<std::string>(), "teamA");
  EXPECT_EQ(json["is_active"].As<bool>(), true);
}

UTEST(ParseMassDeactivateRequest, Basic) {
  userver::formats::json::ValueBuilder b;
  b["user_ids"] = std::vector<std::string>{"id1", "id2"};
  auto val = b.ExtractValue();
  auto req = prmanager::models::Parse(
      val, userver::formats::parse::To<MassDeactivateRequest>{});
  EXPECT_EQ(req.user_ids.size(), 2u);
  EXPECT_EQ(req.user_ids[0], "id1");
}
