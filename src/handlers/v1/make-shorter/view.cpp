#include "view.hpp"

#include <fmt/format.h>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
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

  std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                 userver::server::request::RequestContext&) const override {
    auto request_body = userver::formats::json::FromString(request.RequestBody());

    auto        url      = request_body["url"].As< std::optional<std::string> >();
    auto        vip_url  = request_body["vip_key"].As< std::optional<std::string> >();
    uint64_t    ttl      = request_body["time_to_live"].As< uint64_t >(10);
    std::string ttl_unit = request_body["time_to_live_unit"].As< std::string >("HOURS");

    if (!url.has_value()) {
      auto& response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::ToString(userver::formats::json::MakeObject("detail", "url has no value"));
    }

    // if (vip_url.has_value()) {
    //   response = HandleVipUrl(std::move(url.value()),
    //                           std::move(vip_url.value()),
    //                           std::move(ttl),
    //                           std::move(ttl_unit),
    //                           request);
    // } else {
    //   response = HandleCommonUrl(std::move(url.value()));
    // }

    userver::formats::json::ValueBuilder response = ( vip_url.has_value() ? 
                                                      HandleVipUrl(url.value(),
                                                                   vip_url.value(),
                                                                   ttl,
                                                                   ttl_unit,
                                                                   request)
                                                      : 
                                                      HandleCommonUrl(url.value())
                                                    );
    
    return userver::formats::json::ToString(response.ExtractValue());
  }

  userver::storages::postgres::ClusterPtr pg_cluster_;

 private:
  userver::formats::json::ValueBuilder HandleCommonUrl(const std::string_view& url) const {
    auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "WITH ins AS ( "
        "INSERT INTO url_shortener.urls(url) VALUES($1) "
        "ON CONFLICT DO NOTHING "
        "RETURNING urls.id "
      ") "
      "SELECT id FROM url_shortener.urls WHERE url = $1 "
      "UNION ALL "
      "SELECT id FROM ins ",
      url);

    userver::formats::json::ValueBuilder response;
    response["short_url"] = fmt::format("http://localhost:8080/{}",
                                        result.AsSingleRow<std::string>());

    return response;
  }

  userver::formats::json::ValueBuilder HandleVipUrl(const std::string& url, 
                                                    const std::string& vip_url,
                                                    const uint64_t& ttl,
                                                    const std::string& ttl_unit,
                                                    const userver::server::http::HttpRequest& request
                                                    ) const {

    auto duration = ConvertToSeconds(ttl, ttl_unit);
    if (!duration.has_value()) {
      auto& response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("detail", "Invalid input time parameters");
    }

    auto expiration_time = userver::utils::datetime::Now() + duration.value();
    {
      auto already_exist = pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "WITH updated AS ( "
              "UPDATE url_shortener.urls "
              "SET expiration_time = GREATEST(expiration_time, $3) "
              "WHERE id = $1 AND url = $2 "
              "RETURNING id "
          ") "
          "SELECT id FROM updated ",
          vip_url, url, expiration_time
      );

      if (already_exist.Size() != 0) {
        userver::formats::json::ValueBuilder response;
        response["short_url"] = fmt::format("http://localhost:8080/{}", vip_url);
        return response;
      }
    }

    { // Проверка на то, что vip-ключ уже существует 
      auto exist = pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "SELECT url "
          "FROM url_shortener.urls "
          "WHERE id = $1 AND ($2 < expiration_time OR expiration_time IS NULL)",
          vip_url, userver::utils::datetime::Now()
      );

      if (static_cast<int>(exist.Size()) != 0) {
        auto& response = request.GetHttpResponse();
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::MakeObject("detail", "VIP key already exists");
      }
    }

    try {
      auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO url_shortener.urls (id, url, expiration_time) "
        "VALUES($1, $2, $3) "
        "ON CONFLICT (id) "
        "DO UPDATE SET url = $2, expiration_time = $3 "
        "RETURNING urls.id ",
        vip_url, url, expiration_time
      );
      userver::formats::json::ValueBuilder response;
      response["short_url"] = fmt::format("http://localhost:8080/{}", result.AsSingleRow<std::string>());
      return response;
    } catch (...) {
      throw;
    }
    return {};
  }

  std::optional <std::chrono::seconds> ConvertToSeconds(const uint64_t& time_to_live, 
                                                        const std::string& time_to_live_unit) const {
    std::chrono::seconds result;
    static constexpr std::chrono::hours hours_in_day{24}; 
    static constexpr std::chrono::hours max_duration{48};

    if (time_to_live_unit == "SECONDS") {
      result = std::chrono::seconds(time_to_live);
    } else if (time_to_live_unit == "MINUTES") {
      result = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(time_to_live));
    } else if (time_to_live_unit == "HOURS") {
      result = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(time_to_live));
    } else if (time_to_live_unit == "DAYS") {
      result = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(hours_in_day * time_to_live)); 
    } else {
      return std::nullopt;
    }
    if (std::chrono::duration_cast<std::chrono::seconds>(max_duration) < result) {
      return std::nullopt;
    }
    return result;
  }
};

}  // namespace

void AppendUrlShortener(userver::components::ComponentList& component_list) {
  component_list.Append<UrlShortener>();
}

}  // namespace url_shortener
