#include "view.hpp"

#include <fmt/format.h>

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

namespace url_shortener {

namespace {

class RedirectToLong final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-redirect-to-long";

  RedirectToLong(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("postgres-db-1")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    const auto& id = request.GetPathArg("id");

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT url FROM url_shortener.urls "
        "WHERE id = $1 ",
        id);

    auto& response = request.GetHttpResponse();

    if (result.IsEmpty()) {
      response.SetStatus(userver::server::http::HttpStatus::kNotFound);
      return {};
    }

    response.SetHeader("Location", result.AsSingleRow<std::string>());
    response.SetStatus(userver::server::http::HttpStatus::kMovedPermanently);

    return {};
  }

  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendRedirectToLong(userver::components::ComponentList& component_list) {
  component_list.Append<RedirectToLong>();
}

}  // namespace url_shortener
