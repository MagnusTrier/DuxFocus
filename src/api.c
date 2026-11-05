#include "api.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CURLM *g_multi = NULL;

static void ensure_curl(void) {
  if (g_multi)
    return;
  curl_global_init(CURL_GLOBAL_DEFAULT);
  g_multi = curl_multi_init();
}

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *user) {
  size_t real = size * nmemb;
  api_request_t *req = user;
  char *grown = realloc(req->body, req->body_len + real + 1);
  if (!grown)
    return 0; // signals error to curl
  req->body = grown;
  memcpy(req->body + req->body_len, ptr, real);
  req->body_len += real;
  req->body[req->body_len] = '\0';
  return real;
}

bool api_request_start(api_request_t *req, const char *url) {
  ensure_curl();
  memset(req, 0, sizeof(*req));

  req->easy = curl_easy_init();
  if (!req->easy)
    return false;

  curl_easy_setopt(req->easy, CURLOPT_URL, url);
  curl_easy_setopt(req->easy, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(req->easy, CURLOPT_WRITEDATA, req);
  curl_easy_setopt(req->easy, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(req->easy, CURLOPT_USERAGENT, "dux/1.0");

  if (curl_multi_add_handle(g_multi, req->easy) != CURLM_OK) {
    curl_easy_cleanup(req->easy);
    req->easy = NULL;
    return false;
  }
  req->state = API_IN_FLIGHT;
  return true;
}

void api_request_poll(api_request_t *req) {
  if (req->state != API_IN_FLIGHT)
    return;

  int still_running = 0;
  curl_multi_perform(g_multi, &still_running);
  if (still_running > 0)
    return; // not done yet, try next frame

  struct CURLMsg *msg;
  int left;
  while ((msg = curl_multi_info_read(g_multi, &left))) {
    if (msg->easy_handle != req->easy)
      continue;
    req->result = msg->data.result;
    if (req->result == CURLE_OK) {
      curl_easy_getinfo(req->easy, CURLINFO_RESPONSE_CODE, &req->http_code);
      fprintf(stderr, "got %zu bytes, HTTP %ld\n", req->body_len,
              req->http_code);
      fprintf(stderr, "body: %.*s\n", (int)req->body_len, req->body);
      req->state = API_DONE;
    } else {
      req->state = API_FAILED;
    }
  }
}

void api_request_free(api_request_t *req) {
  if (req->easy) {
    curl_multi_remove_handle(g_multi, req->easy);
    curl_easy_cleanup(req->easy);
    req->easy = NULL;
  }
  free(req->body);
  req->body = NULL;
  req->body_len = 0;
  req->state = API_IDLE;
}
