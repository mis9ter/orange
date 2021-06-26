// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "mongoose.h"

#define WIFI_SSID "WIFI_NETWORK"   // SET THIS!
#define WIFI_PASS "WIFI_PASSWORD"  // SET THIS!
#define FS_ROOT "/spiffs"

// Event handler for an server (accepted) connection. Implemented endpoints:
//    /api/stats  - return JSON object with ESP32 stats (free RAM)
//    any other   - serve files from the filesystem
static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = ev_data;
    if (mg_http_match_uri(hm, "/api/stats")) {
      mg_http_reply(c, 200, "", "{\"ram\": %lu}\n", xPortGetFreeHeapSize());
    } else {
      struct mg_http_serve_opts opts = {.root_dir = FS_ROOT};
      mg_http_serve_dir(c, hm, &opts);
    }
  }
}

// SPIFFS is flat, so tell Mongoose that the FS root is a directory
// This cludge is not required for filesystems with directory support
bool mg_is_dir(const char *path) {
  return strcmp(path, FS_ROOT) == 0;
}

void app_main(void) {
  // Mount filesystem
  esp_vfs_spiffs_conf_t conf = {
      .base_path = FS_ROOT, .max_files = 20, .format_if_mount_failed = true};
  int res = esp_vfs_spiffs_register(&conf);
  LOG(res == ESP_OK ? LL_INFO : LL_ERROR, ("FS %s, %d", conf.base_path, res));
  mg_file_printf(FS_ROOT "/hello.txt", "%s", "hello from ESP");

  // Setup wifi. This function is implemented in wifi.c
  // It blocks until connected to the configured WiFi network
  void wifi_init(const char *ssid, const char *pass);
  wifi_init(WIFI_SSID, WIFI_PASS);

  // Connected to WiFi, now start HTTP server
  struct mg_mgr mgr;
  mg_log_set("3");
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, "http://0.0.0.0:80", cb, &mgr);  // Listening server
  LOG(LL_INFO, ("Starting Mongoose web server v%s", MG_VERSION));
  for (;;) mg_mgr_poll(&mgr, 1000);  // Infinite event loop
}
