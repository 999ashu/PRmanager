#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/storages/postgres/component.hpp> 

#include <userver/utils/daemon_run.hpp>

#include "handlers.hpp"

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
                              .Append<userver::server::handlers::Ping>()
                              .Append<userver::components::TestsuiteSupport>()
                              .Append<userver::components::HttpClient>()
                              .Append<userver::clients::dns::Component>()
                              .Append<userver::server::handlers::TestsControl>()
                              .Append<userver::components::Postgres>("postgres-db-1")
                              .Append<prmanager::handlers::TeamAddHandler>()
                              .Append<prmanager::handlers::TeamGetHandler>()
                              .Append<prmanager::handlers::UserSetIsActiveHandler>()
                              .Append<prmanager::handlers::PullRequestCreateHandler>()
                              .Append<prmanager::handlers::PullRequestMergeHandler>()
                              .Append<prmanager::handlers::PullRequestReassignHandler>()
                              .Append<prmanager::handlers::UserGetReviewHandler>()
                              .Append<prmanager::handlers::MassDeactivateHandler>()
                              .Append<prmanager::handlers::StatsHandler>()
        ;

    return userver::utils::DaemonMain(argc, argv, component_list);
}
