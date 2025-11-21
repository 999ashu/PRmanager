#pragma once

#include <string>
#include <vector>
#include <optional>
#include <userver/formats/json.hpp>
#include <userver/utils/time_of_day.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

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

struct User {
    std::string user_id;
    std::string username;
    std::string team_name;
    bool is_active;
};

struct PullRequest {
    std::string pull_request_id;
    std::string pull_request_name;
    std::string author_id;
    std::string status;
    std::vector<std::string> assigned_reviewers;
    std::optional<std::string> created_at;
    std::optional<std::string> merged_at;
};

struct PullRequestShort {
    std::string pull_request_id;
    std::string pull_request_name;
    std::string author_id;
    std::string status;
};

struct MassDeactivateRequest {
    std::vector<std::string> user_ids;
};

struct MassDeactivateResponse {
    int deactivated_count;
};

struct StatsResponse {
    int teams_count;
    int users_count;
    int prs_count;
};

TeamMember Parse(const userver::formats::json::Value& json,
                 userver::formats::parse::To<TeamMember>);

Team Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<Team>);

userver::formats::json::Value Serialize(const Team& team,
                                        userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value Serialize(const User& user,
                                        userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value Serialize(const PullRequest& pr,
                                        userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value Serialize(const PullRequestShort& pr,
                                        userver::formats::serialize::To<userver::formats::json::Value>);

MassDeactivateRequest Parse(const userver::formats::json::Value& json,
                            userver::formats::parse::To<MassDeactivateRequest>);

userver::formats::json::Value Serialize(const MassDeactivateResponse& response,
                                        userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value Serialize(const StatsResponse& response,
                                        userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace prmanager::models
