#include <cstring>
#include <unity.h>
#include "../../lib/cooling_feature/src/web/WebDashboardPage.h"

namespace {

void test_dashboard_assets_are_inlined()
{
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "<style>"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "<script>"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "--bg: #f5f7f8"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "window.DeviceWeb"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "window.SystemNetwork"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "window.SystemOta"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "window.CoolingApi"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "window.CoolingChart"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "window.CoolingPrediction"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "settingsForm"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "temperatureChart"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "chartDownloadPng"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "Filtered temperature"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "fetchWithTimeout"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "renderStatusUnavailable"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "/api/history"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "/api/history.csv"));
}

void test_embedded_pages_do_not_depend_on_asset_routes()
{
  TEST_ASSERT_NULL(std::strstr(kIndexHtml, "<link rel=\"stylesheet\""));
  TEST_ASSERT_NULL(std::strstr(kIndexHtml, "<script src=\""));
  TEST_ASSERT_NULL(std::strstr(kIndexHtml, "/assets/"));
}

void test_page_routes_resolve_to_embedded_html()
{
  TEST_ASSERT_EQUAL_PTR(kIndexHtml, webPageForPath("/"));
  TEST_ASSERT_EQUAL_PTR(kIndexHtml, webPageForPath("/dashboard.html"));
  TEST_ASSERT_EQUAL_PTR(kIndexHtml, webPageForPath("/apps/cooling/dashboard"));
  TEST_ASSERT_EQUAL_PTR(kIndexHtml,
                        webPageForPath("/apps/cooling/dashboard.html"));
}

void test_unknown_page_route_returns_null()
{
  TEST_ASSERT_NULL(webPageForPath("/assets/app.css"));
  TEST_ASSERT_NULL(webPageForPath("/core/app.css"));
  TEST_ASSERT_NULL(webPageForPath("/demo"));
  TEST_ASSERT_NULL(webPageForPath("/dev"));
  TEST_ASSERT_NULL(webPageForPath("/missing"));
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_dashboard_assets_are_inlined);
  RUN_TEST(test_embedded_pages_do_not_depend_on_asset_routes);
  RUN_TEST(test_page_routes_resolve_to_embedded_html);
  RUN_TEST(test_unknown_page_route_returns_null);
  return UNITY_END();
}
