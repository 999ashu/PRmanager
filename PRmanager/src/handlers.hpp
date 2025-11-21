#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace prmanager::handlers {

class TeamAddHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-team-add";

    TeamAddHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

class TeamGetHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-team-get";

    TeamGetHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

class UserSetIsActiveHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-set-is-active";

    UserSetIsActiveHandler(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

class PullRequestCreateHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-pr-create";

    PullRequestCreateHandler(const userver::components::ComponentConfig& config,
                             const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

class PullRequestMergeHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-pr-merge";

    PullRequestMergeHandler(const userver::components::ComponentConfig& config,
                            const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

class PullRequestReassignHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-pr-reassign";

    PullRequestReassignHandler(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

class UserGetReviewHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-get-review";

    UserGetReviewHandler(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

class MassDeactivateHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-mass-deactivate";

    MassDeactivateHandler(const userver::components::ComponentConfig& config,
                          const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

class StatsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-stats";

    StatsHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace prmanager::handlers
