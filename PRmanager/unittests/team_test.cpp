#include <string>
#include <vector>

#include <userver/utest/utest.hpp>
#include <userver/formats/json.hpp>

#include "models/team.hpp"

using prmanager::models::Team;
using prmanager::models::TeamMember;

UTEST(TeamSerializeParse, Basic) {
  TeamMember m1{"id1", "alice", true};
  TeamMember m2{"id2", "bob", false};
  Team team{"teamA", {m1, m2}};
  auto json = prmanager::models::Serialize(
      team, userver::formats::serialize::To<userver::formats::json::Value>{});
  EXPECT_EQ(json["team_name"].As<std::string>(), "teamA");

  auto parsed = prmanager::models::Parse(
      json, userver::formats::parse::To<Team>{});
  EXPECT_EQ(parsed.team_name, "teamA");
  EXPECT_EQ(parsed.members.size(), 2u);
  EXPECT_EQ(parsed.members[1].username, "bob");
}
