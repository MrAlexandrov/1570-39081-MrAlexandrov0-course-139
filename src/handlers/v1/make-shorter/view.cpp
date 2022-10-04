#include "view.hpp"

#include <fmt/format.h>

#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

namespace url_shortener {

namespace {

class UrlShortener final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-make-shorter";

  UrlShortener(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("postgres-db-1")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    auto request_body =
        userver::formats::json::FromString(request.RequestBody());

    auto url = request_body["url"].As<std::optional<std::string>>();
    if (!url.has_value()) {
      auto& response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
      return {};
    }

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "WITH ins AS ( "
        "INSERT INTO url_shortener.urls(url) VALUES($1) "
        "ON CONFLICT DO NOTHING "
        "RETURNING urls.id "
        ") "
        "SELECT id FROM url_shortener.urls WHERE url = $1 "
        "UNION ALL "
        "SELECT id FROM ins",
        url.value());

    userver::formats::json::ValueBuilder response;
    response["short_url"] =
        fmt::format("http://localhost:8080/{}",
                    result.AsSingleRow<std::string>());

    return userver::formats::json::ToString(response.ExtractValue());
  }

  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendUrlShortener(userver::components::ComponentList& component_list) {
  component_list.Append<UrlShortener>();
}

}  // namespace url_shortener
