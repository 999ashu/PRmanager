#include "models.hpp"

namespace prmanager::models {

TeamMember Parse(const userver::formats::json::Value& json,
                 userver::formats::parse::To<TeamMember>) {
    return TeamMember{
        json["user_id"].As<std::string>(),
        json["username"].As<std::string>(),
        json["is_active"].As<bool>()
    };
}

Team Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<Team>) {
    return Team{
        json["team_name"].As<std::string>(),
        json["members"].As<std::vector<TeamMember>>()
    };
}

userver::formats::json::Value Serialize(const TeamMember& member,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["user_id"] = member.user_id;
    builder["username"] = member.username;
    builder["is_active"] = member.is_active;
    return builder.ExtractValue();
}

userver::formats::json::Value Serialize(const Team& team,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["team_name"] = team.team_name;
    builder["members"] = team.members;
    return builder.ExtractValue();
}

userver::formats::json::Value Serialize(const User& user,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["user_id"] = user.user_id;
    builder["username"] = user.username;
    builder["team_name"] = user.team_name;
    builder["is_active"] = user.is_active;
    return builder.ExtractValue();
}

userver::formats::json::Value Serialize(const PullRequest& pr,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["pull_request_id"] = pr.pull_request_id;
    builder["pull_request_name"] = pr.pull_request_name;
    builder["author_id"] = pr.author_id;
    builder["status"] = pr.status;
    builder["assigned_reviewers"] = pr.assigned_reviewers;
    if (pr.created_at) builder["createdAt"] = *pr.created_at;
    if (pr.merged_at) builder["mergedAt"] = *pr.merged_at;
    return builder.ExtractValue();
}

userver::formats::json::Value Serialize(const PullRequestShort& pr,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["pull_request_id"] = pr.pull_request_id;
    builder["pull_request_name"] = pr.pull_request_name;
    builder["author_id"] = pr.author_id;
    builder["status"] = pr.status;
    return builder.ExtractValue();
}

MassDeactivateRequest Parse(const userver::formats::json::Value& json,
                            userver::formats::parse::To<MassDeactivateRequest>) {
    return MassDeactivateRequest{
        json["user_ids"].As<std::vector<std::string>>()
    };
}

userver::formats::json::Value Serialize(const MassDeactivateResponse& response,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["deactivated_count"] = response.deactivated_count;
    return builder.ExtractValue();
}

userver::formats::json::Value Serialize(const StatsResponse& response,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["teams_count"] = response.teams_count;
    builder["users_count"] = response.users_count;
    builder["prs_count"] = response.prs_count;
    return builder.ExtractValue();
}

}  // namespace prmanager::models
