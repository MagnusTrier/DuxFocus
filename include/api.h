#ifndef API_H
#define API_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  API_IDLE,
  API_IN_FLIGHT,
  API_DONE,
  API_FAILED,
} api_state_t;

typedef struct {
  api_state_t state;
  char *body;
  size_t body_len;
  long http_code;
  int result; // CURLcode, but exposed as int to avoid curl.h
  void *easy; // CURL*, opaque — do not touch outside api.c
} api_request_t;

bool api_request_start(api_request_t *req, const char *url);
void api_request_poll(api_request_t *req);
void api_request_free(api_request_t *req);

#endif
