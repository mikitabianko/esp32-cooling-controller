#include <cstring>
#include <unity.h>
#include "web/WebDashboardPage.h"

namespace {

void test_dashboard_assets_are_inlined()
{
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "<style>"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "<script>"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "--bg: #f5f7f8"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "window.CoolingWeb"));
  TEST_ASSERT_NOT_NULL(std::strstr(kIndexHtml, "settingsForm"));
}

void test_dev_assets_are_inlined()
{
  TEST_ASSERT_NOT_NULL(std::strstr(kDevHtml, "<style>"));
  TEST_ASSERT_NOT_NULL(std::strstr(kDevHtml, "<script>"));
  TEST_ASSERT_NOT_NULL(std::strstr(kDevHtml, "window.CoolingWeb"));
  TEST_ASSERT_NOT_NULL(std::strstr(kDevHtml, "devForm"));
}

void test_embedded_pages_do_not_depend_on_asset_routes()
{
  TEST_ASSERT_NULL(std::strstr(kIndexHtml, "<link rel=\"stylesheet\""));
  TEST_ASSERT_NULL(std::strstr(kIndexHtml, "<script src=\""));
  TEST_ASSERT_NULL(std::strstr(kIndexHtml, "/assets/"));
  TEST_ASSERT_NULL(std::strstr(kDevHtml, "<link rel=\"stylesheet\""));
  TEST_ASSERT_NULL(std::strstr(kDevHtml, "<script src=\""));
  TEST_ASSERT_NULL(std::strstr(kDevHtml, "/assets/"));
}

void test_page_routes_resolve_to_embedded_html()
{
  TEST_ASSERT_EQUAL_PTR(kIndexHtml, webPageForPath("/"));
  TEST_ASSERT_EQUAL_PTR(kIndexHtml, webPageForPath("/dashboard.html"));
  TEST_ASSERT_EQUAL_PTR(kDevHtml, webPageForPath("/dev"));
  TEST_ASSERT_EQUAL_PTR(kDevHtml, webPageForPath("/dev.html"));
}

void test_unknown_page_route_returns_null()
{
  TEST_ASSERT_NULL(webPageForPath("/assets/app.css"));
  TEST_ASSERT_NULL(webPageForPath("/missing"));
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_dashboard_assets_are_inlined);
  RUN_TEST(test_dev_assets_are_inlined);
  RUN_TEST(test_embedded_pages_do_not_depend_on_asset_routes);
  RUN_TEST(test_page_routes_resolve_to_embedded_html);
  RUN_TEST(test_unknown_page_route_returns_null);
  return UNITY_END();
}
