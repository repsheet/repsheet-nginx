#ifndef __NGX_HTTP_REPSHEET_XFF_H__
#define __NGX_HTTP_REPSHEET_XFF_H__

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#if __FreeBSD__
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "ngx_http_repsheet_module.h"

ngx_table_elt_t *x_forwarded_for(ngx_http_request_t *r);
ngx_table_elt_t *extract_proxy_header(ngx_http_request_t *r, repsheet_loc_conf_t *loc_conf);
int validate_actor_address(ngx_http_request_t *r, ngx_table_elt_t *xff, char *address);
int derive_actor_address(ngx_http_request_t *r, repsheet_loc_conf_t *loc_conf, char *address);

#endif
