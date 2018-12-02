#ifndef __NGX_HTTP_REPSHEET_LOOKUP_H__
#define __NGX_HTTP_REPSHEET_LOOKUP_H__

#include "ngx_http_repsheet_module.h"

ngx_int_t lookup_ip(ngx_http_request_t *r, repsheet_main_conf_t *main_conf, repsheet_loc_conf_t *loc_conf);

#endif
