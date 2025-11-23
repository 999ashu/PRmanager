#include "mass_deactivate.hpp"
#include "../models/stats.hpp"
#include "../models/user.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>

#include <random>

namespace prmanager::handlers {

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
    return userver::formats::json::ToString(models::Serialize(
        response,
        userver::formats::serialize::To<userver::formats::json::Value>{}));

  } catch (const std::exception& e) {
    trx.Rollback();
    throw;
  }
}

}  // namespace prmanager::handlers
