#include <userver/utest/utest.hpp>
#include <userver/formats/json.hpp>

#include "models/stats.hpp"

using prmanager::models::StatsResponse;

UTEST(StatsSerialize, Basic) {
  StatsResponse s{10, 20, 30};
  auto json = prmanager::models::Serialize(
      s, userver::formats::serialize::To<userver::formats::json::Value>{});
  EXPECT_EQ(json["teams_count"].As<int>(), 10);
  EXPECT_EQ(json["users_count"].As<int>(), 20);
  EXPECT_EQ(json["prs_count"].As<int>(), 30);
}
