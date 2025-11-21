#include "handlers.hpp"
#include "models.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>
#include <userver/utils/rand.hpp>

#include <algorithm>
#include <random>

namespace prmanager::handlers {

namespace {

struct ErrorResponse {
  std::string code;
  std::string message;
};

inline userver::formats::json::Value BuildErrorResponse(
    const std::string& code, const std::string& message) {
  userver::formats::json::ValueBuilder builder;
  userver::formats::json::ValueBuilder error_builder;
  error_builder["code"] = code;
  error_builder["message"] = message;
  builder["error"] = error_builder.ExtractValue();
  return builder.ExtractValue();
}

}  // namespace

TeamAddHandler::TeamAddHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string TeamAddHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto team = body.As<models::Team>();

  auto trx = pg_cluster_->Begin(
      "team_add", userver::storages::postgres::ClusterHostType::kMaster, {});

  try {
    auto res = trx.Execute("SELECT 1 FROM prmanager.teams WHERE name = $1",
                           team.team_name);
    if (!res.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::ToString(
          BuildErrorResponse("TEAM_EXISTS", "team_name already exists"));
    }

    trx.Execute("INSERT INTO prmanager.teams (name) VALUES ($1)",
                team.team_name);

    for (const auto& member : team.members) {
      trx.Execute(
          "INSERT INTO prmanager.users (id, username, team_name, is_active) "
          "VALUES ($1, $2, $3, $4) "
          "ON CONFLICT (id) DO UPDATE SET username = $2, team_name = $3, "
          "is_active = $4",
          member.user_id, member.username, team.team_name, member.is_active);
    }

    trx.Commit();
  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
  userver::formats::json::ValueBuilder response;
  response["team"] = team;
  return userver::formats::json::ToString(response.ExtractValue());
}

TeamGetHandler::TeamGetHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string TeamGetHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto& team_name = request.GetArg("team_name");
  if (team_name.empty()) {
    throw userver::server::handlers::ClientError(
        userver::server::handlers::ExternalBody{"Missing team_name"});
  }

  auto res_team = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT name FROM prmanager.teams WHERE name = $1", team_name);
  if (res_team.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return userver::formats::json::ToString(
        BuildErrorResponse("NOT_FOUND", "Team not found"));
  }

  auto res_users =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           "SELECT id, username, is_active FROM "
                           "prmanager.users WHERE team_name = $1",
                           team_name);

  models::Team team;
  team.team_name = team_name;
  for (const auto& row : res_users) {
    team.members.push_back(models::TeamMember{row["id"].As<std::string>(),
                                              row["username"].As<std::string>(),
                                              row["is_active"].As<bool>()});
  }

  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder(team).ExtractValue());
}

UserSetIsActiveHandler::UserSetIsActiveHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string UserSetIsActiveHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto user_id = body["user_id"].As<std::string>();
  const auto is_active = body["is_active"].As<bool>();

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "UPDATE prmanager.users SET is_active = $1 WHERE id = $2 "
      "RETURNING id, username, team_name, is_active",
      is_active, user_id);

  if (res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return userver::formats::json::ToString(
        BuildErrorResponse("NOT_FOUND", "User not found"));
  }

  const auto& row = res[0];
  models::User user{
      row["id"].As<std::string>(), row["username"].As<std::string>(),
      row["team_name"].As<std::string>(), row["is_active"].As<bool>()};

  userver::formats::json::ValueBuilder response;
  response["user"] = user;
  return userver::formats::json::ToString(response.ExtractValue());
}

PullRequestCreateHandler::PullRequestCreateHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string PullRequestCreateHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto pr_id = body["pull_request_id"].As<std::string>();
  const auto pr_name = body["pull_request_name"].As<std::string>();
  const auto author_id = body["author_id"].As<std::string>();

  auto trx = pg_cluster_->Begin(
      "pr_create", userver::storages::postgres::ClusterHostType::kMaster, {});

  try {
    auto res_pr = trx.Execute(
        "SELECT 1 FROM prmanager.pull_requests WHERE id = $1", pr_id);
    if (!res_pr.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::ToString(
          BuildErrorResponse("PR_EXISTS", "PR id already exists"));
    }

    auto res_author = trx.Execute(
        "SELECT team_name FROM prmanager.users WHERE id = $1", author_id);
    if (res_author.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return userver::formats::json::ToString(
          BuildErrorResponse("NOT_FOUND", "Author not found"));
    }
    const auto team_name = res_author[0]["team_name"].As<std::string>();

    auto res_candidates = trx.Execute(
        "SELECT id FROM prmanager.users WHERE team_name = $1 AND is_active = "
        "TRUE AND id != $2",
        team_name, author_id);

    std::vector<std::string> candidates;
    for (const auto& row : res_candidates) {
      candidates.push_back(row["id"].As<std::string>());
    }

    std::vector<std::string> reviewers;
    if (candidates.size() <= 2) {
      reviewers = candidates;
    } else {
      std::sample(candidates.begin(), candidates.end(),
                  std::back_inserter(reviewers), 2,
                  std::mt19937{std::random_device{}()});
    }

    trx.Execute(
        "INSERT INTO prmanager.pull_requests (id, name, author_id, status) "
        "VALUES ($1, $2, $3, 'OPEN')",
        pr_id, pr_name, author_id);

    for (const auto& reviewer : reviewers) {
      trx.Execute(
          "INSERT INTO prmanager.reviewers (pull_request_id, reviewer_id) "
          "VALUES ($1, $2)",
          pr_id, reviewer);
    }

    trx.Commit();

    models::PullRequest pr;
    pr.pull_request_id = pr_id;
    pr.pull_request_name = pr_name;
    pr.author_id = author_id;
    pr.status = "OPEN";
    pr.assigned_reviewers = reviewers;

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    userver::formats::json::ValueBuilder response;
    response["pr"] = pr;
    return userver::formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }
}

PullRequestMergeHandler::PullRequestMergeHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string PullRequestMergeHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto pr_id = body["pull_request_id"].As<std::string>();

  auto trx = pg_cluster_->Begin(
      "pr_merge", userver::storages::postgres::ClusterHostType::kMaster, {});

  try {
    auto res_pr = trx.Execute(
        "UPDATE prmanager.pull_requests SET status = 'MERGED', merged_at = "
        "COALESCE(merged_at, NOW()) "
        "WHERE id = $1 RETURNING id, name, author_id, status, merged_at",
        pr_id);

    if (res_pr.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return userver::formats::json::ToString(
          BuildErrorResponse("NOT_FOUND", "PR not found"));
    }

    auto res_reviewers = trx.Execute(
        "SELECT reviewer_id FROM prmanager.reviewers WHERE pull_request_id = "
        "$1",
        pr_id);
    std::vector<std::string> reviewers;
    for (const auto& row : res_reviewers) {
      reviewers.push_back(row["reviewer_id"].As<std::string>());
    }

    trx.Commit();

    const auto& row = res_pr[0];
    models::PullRequest pr;
    pr.pull_request_id = row["id"].As<std::string>();
    pr.pull_request_name = row["name"].As<std::string>();
    pr.author_id = row["author_id"].As<std::string>();
    pr.status = row["status"].As<std::string>();
    pr.assigned_reviewers = reviewers;
    pr.merged_at = userver::utils::datetime::Timestring(
        row["merged_at"]
            .As<userver::storages::postgres::TimePointTz>()
            .GetUnderlying());

    userver::formats::json::ValueBuilder response;
    response["pr"] = pr;
    return userver::formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }
}

PullRequestReassignHandler::PullRequestReassignHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string PullRequestReassignHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto pr_id = body["pull_request_id"].As<std::string>();
  const auto old_user_id = body["old_user_id"].As<std::string>();

  auto trx = pg_cluster_->Begin(
      "pr_reassign", userver::storages::postgres::ClusterHostType::kMaster, {});

  try {
    auto res_pr = trx.Execute(
        "SELECT status, author_id FROM prmanager.pull_requests WHERE id = $1",
        pr_id);
    if (res_pr.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return userver::formats::json::ToString(
          BuildErrorResponse("NOT_FOUND", "PR not found"));
    }
    if (res_pr[0]["status"].As<std::string>() == "MERGED") {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::ToString(
          BuildErrorResponse("PR_MERGED", "cannot reassign on merged PR"));
    }
    const auto author_id = res_pr[0]["author_id"].As<std::string>();

    auto res_reviewer = trx.Execute(
        "SELECT 1 FROM prmanager.reviewers WHERE pull_request_id = $1 AND "
        "reviewer_id = $2",
        pr_id, old_user_id);
    if (res_reviewer.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::ToString(BuildErrorResponse(
          "NOT_ASSIGNED", "reviewer is not assigned to this PR"));
    }

    auto res_user = trx.Execute(
        "SELECT team_name FROM prmanager.users WHERE id = $1", old_user_id);
    if (res_user.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return userver::formats::json::ToString(
          BuildErrorResponse("NOT_FOUND", "User not found"));
    }
    const auto team_name = res_user[0]["team_name"].As<std::string>();

    auto res_current_reviewers = trx.Execute(
        "SELECT reviewer_id FROM prmanager.reviewers WHERE pull_request_id = "
        "$1",
        pr_id);
    std::vector<std::string> current_reviewers;
    for (const auto& row : res_current_reviewers) {
      current_reviewers.push_back(row["reviewer_id"].As<std::string>());
    }

    auto res_candidates = trx.Execute(
        "SELECT id FROM prmanager.users WHERE team_name = $1 AND is_active = "
        "TRUE AND id != $2",
        team_name, author_id);

    std::vector<std::string> candidates;
    for (const auto& row : res_candidates) {
      std::string uid = row["id"].As<std::string>();
      bool is_reviewer = false;
      for (const auto& r : current_reviewers) {
        if (r == uid) {
          is_reviewer = true;
          break;
        }
      }
      if (!is_reviewer) {
        candidates.push_back(uid);
      }
    }

    if (candidates.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::ToString(BuildErrorResponse(
          "NO_CANDIDATE", "no active replacement candidate in team"));
    }

    std::string new_reviewer_id;
    std::sample(candidates.begin(), candidates.end(), &new_reviewer_id, 1,
                std::mt19937{std::random_device{}()});

    trx.Execute(
        "DELETE FROM prmanager.reviewers WHERE pull_request_id = $1 AND "
        "reviewer_id = $2",
        pr_id, old_user_id);
    trx.Execute(
        "INSERT INTO prmanager.reviewers (pull_request_id, reviewer_id) VALUES "
        "($1, $2)",
        pr_id, new_reviewer_id);

    trx.Commit();

    std::replace(current_reviewers.begin(), current_reviewers.end(),
                 old_user_id, new_reviewer_id);

    models::PullRequest pr;
    pr.pull_request_id = pr_id;
    auto res_pr_details = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        "SELECT name FROM prmanager.pull_requests WHERE id = $1", pr_id);
    pr.pull_request_name = res_pr_details[0]["name"].As<std::string>();
    pr.author_id = author_id;
    pr.status = "OPEN";
    pr.assigned_reviewers = current_reviewers;

    userver::formats::json::ValueBuilder response;
    response["pr"] = pr;
    response["replaced_by"] = new_reviewer_id;
    return userver::formats::json::ToString(response.ExtractValue());

  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }
}

UserGetReviewHandler::UserGetReviewHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string UserGetReviewHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto& user_id = request.GetArg("user_id");
  if (user_id.empty()) {
    throw userver::server::handlers::ClientError(
        userver::server::handlers::ExternalBody{"Missing user_id"});
  }

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT pr.id, pr.name, pr.author_id, pr.status "
      "FROM prmanager.pull_requests pr "
      "JOIN prmanager.reviewers r ON pr.id = r.pull_request_id "
      "WHERE r.reviewer_id = $1",
      user_id);

  std::vector<models::PullRequestShort> prs;
  for (const auto& row : res) {
    prs.push_back(models::PullRequestShort{
        row["id"].As<std::string>(), row["name"].As<std::string>(),
        row["author_id"].As<std::string>(), row["status"].As<std::string>()});
  }

  userver::formats::json::ValueBuilder response;
  response["user_id"] = user_id;
  response["pull_requests"] = prs;
  return userver::formats::json::ToString(response.ExtractValue());
}

MassDeactivateHandler::MassDeactivateHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string MassDeactivateHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto body = userver::formats::json::FromString(request.RequestBody());
  const auto req = body.As<models::MassDeactivateRequest>();

  auto trx = pg_cluster_->Begin(
      "mass_deactivate", userver::storages::postgres::ClusterHostType::kMaster,
      {});

  try {
    auto res_update = trx.Execute(
        "UPDATE prmanager.users SET is_active = FALSE WHERE id = ANY($1) "
        "RETURNING id",
        req.user_ids);

    for (const auto& row_u : res_update) {
      std::string user_id = row_u["id"].As<std::string>();

      auto res_prs = trx.Execute(
          "SELECT pr.id, pr.author_id, u.team_name "
          "FROM prmanager.pull_requests pr "
          "JOIN prmanager.reviewers r ON pr.id = r.pull_request_id "
          "JOIN prmanager.users u ON u.id = $1 "
          "WHERE r.reviewer_id = $1 AND pr.status = 'OPEN'",
          user_id);

      for (const auto& row_pr : res_prs) {
        std::string pr_id = row_pr["id"].As<std::string>();
        std::string author_id = row_pr["author_id"].As<std::string>();
        std::string team_name = row_pr["team_name"].As<std::string>();

        auto res_curr = trx.Execute(
            "SELECT reviewer_id FROM prmanager.reviewers WHERE pull_request_id "
            "= $1",
            pr_id);
        std::vector<std::string> current_reviewers;
        for (const auto& r : res_curr) {
          current_reviewers.push_back(r["reviewer_id"].As<std::string>());
        }

        auto res_cand = trx.Execute(
            "SELECT id FROM prmanager.users "
            "WHERE team_name = $1 AND is_active = TRUE AND id != $2 "
            "AND NOT (id = ANY($3))",
            team_name, author_id, current_reviewers);

        if (!res_cand.IsEmpty()) {
          std::vector<std::string> candidates;
          for (const auto& r : res_cand) {
            candidates.push_back(r["id"].As<std::string>());
          }

          std::string new_reviewer;
          std::sample(candidates.begin(), candidates.end(), &new_reviewer, 1,
                      std::mt19937{std::random_device{}()});

          trx.Execute(
              "DELETE FROM prmanager.reviewers WHERE pull_request_id = $1 AND "
              "reviewer_id = $2",
              pr_id, user_id);
          trx.Execute(
              "INSERT INTO prmanager.reviewers (pull_request_id, reviewer_id) "
              "VALUES ($1, $2)",
              pr_id, new_reviewer);
        } else {
          trx.Execute(
              "DELETE FROM prmanager.reviewers WHERE pull_request_id = $1 AND "
              "reviewer_id = $2",
              pr_id, user_id);
        }
      }
    }

    trx.Commit();

    models::MassDeactivateResponse response;
    response.deactivated_count = res_update.Size();
    return userver::formats::json::ToString(
        models::Serialize(response, userver::formats::serialize::To<
                                        userver::formats::json::Value>{}));

  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }
}

StatsHandler::StatsHandler(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string StatsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest&,
    userver::server::request::RequestContext&) const {
  auto res_teams = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT COUNT(*) FROM prmanager.teams");
  auto res_users = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT COUNT(*) FROM prmanager.users");
  auto res_prs = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT COUNT(*) FROM prmanager.pull_requests");

  models::StatsResponse response;
  response.teams_count = res_teams[0][0].As<int>();
  response.users_count = res_users[0][0].As<int>();
  response.prs_count = res_prs[0][0].As<int>();

  return userver::formats::json::ToString(
      models::Serialize(response, userver::formats::serialize::To<
                                      userver::formats::json::Value>{}));
}

}  // namespace prmanager::handlers
