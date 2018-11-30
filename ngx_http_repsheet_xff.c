#include <ngx_http.h>

#include "ngx_http_repsheet_xff.h"

ngx_table_elt_t *x_forwarded_for(ngx_http_request_t *r)
{
  ngx_table_elt_t *xff = NULL;
  ngx_array_t *ngx_array = &r->headers_in.x_forwarded_for;

  if (ngx_array != NULL && ngx_array->nelts > 0) {
    ngx_table_elt_t **first_elt = ngx_array->elts;
    xff = first_elt[0];
  }

  return xff;
}

int remote_address(char *connected_address, char *xff_header, char *address)
{
  if ((connected_address == NULL && xff_header == NULL) || address == NULL) {
    return INVALID;
  }

  int length;
  memset(address, '\0', INET6_ADDRSTRLEN);

  if (xff_header != NULL) {
    char *p;

    for (p = xff_header; p < (xff_header + strlen(xff_header)); p++) {
      if (*p == ' ' || *p == ',') {
        break;
      }
    }

    length = p - xff_header;
    char test_address[length + 1];
    memcpy(test_address, xff_header, length);
    test_address[length] = '\0';

    unsigned char buf[sizeof(struct in_addr)];
    unsigned char buf6[sizeof(struct in6_addr)];

    if (inet_pton(AF_INET, (const char *)test_address, buf) == 1 || inet_pton(AF_INET6, (const char *)test_address, buf6) == 1) {
      memcpy(address, test_address, length);
      return OK;
    } else {
      return INVALID;
    }
  } else {
    memcpy(address, connected_address, strlen(connected_address));
    return OK;
  }
}

int validate_actor_address(ngx_http_request_t *r, ngx_table_elt_t *xff, char *address)
{
  int result;

  if (xff == NULL) {
    memcpy(address, r->connection->addr_text.data, r->connection->addr_text.len);
    address[r->connection->addr_text.len] = '\0';
    return NGX_OK;
  } else {
    result = remote_address((char *)r->connection->addr_text.data, (char*)xff->value.data, address);
    if (result == INVALID) {
      return NGX_DECLINED;
    } else {
      return NGX_OK;
    }
  }
}

ngx_table_elt_t *extract_proxy_header(ngx_http_request_t *r, repsheet_loc_conf_t *loc_conf)
{
  ngx_list_part_t *part;
  ngx_table_elt_t *h;
  ngx_uint_t i;

  part = &r->headers_in.headers.part;
  h = part->elts;

  for (i = 0; /**/; i++) {
    if (i >= part->nelts) {
      if (part->next == NULL) {
	break;
      }

      part = part->next;
      h = part->elts;
      i = 0;
    }

    if (ngx_strncmp(h[i].key.data, loc_conf->proxy_headers_header.data, h[i].key.len) == 0) {
      return &h[i];
    }
  }

  return NULL;
}

int derive_actor_address(ngx_http_request_t *r, repsheet_loc_conf_t *loc_conf, char *address)
{
  int result;
  int alternate = 0;
  ngx_table_elt_t *xff;

  if (ngx_strncmp(loc_conf->proxy_headers_header.data, "X-Forwarded-For", 15) == 0) {
    xff = x_forwarded_for(r);
  } else {
    alternate = 1;
    xff = extract_proxy_header(r, loc_conf);
  }

  result = validate_actor_address(r, xff, address);

  if (result == NGX_DECLINED && alternate && loc_conf->proxy_headers_fallback) {
    return validate_actor_address(r, x_forwarded_for(r), address);
  } else {
    return result;
  }
}
