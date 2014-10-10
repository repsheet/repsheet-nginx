# Configuration

To activate and configure Repsheet you will need to set some
directives. The following list explains what each directive is and
what is does. At the moment, the Repsheet directives all live under
the main configuration section of `nginx.conf`.

* `repsheet <on|off>` - [en|dis]able the module
* `repsheet_ip_lookup <on|off>` - [en|dis]able lookup by IP address
* `repsheet_user_lookup <on|off>` - [en|dis]able lookup by cookie
* `repsheet_proxy_headers <on|off>` - [en|dis]able use of the X-Forwarded-For header
* `repsheet_redis_host <host>` - Redis host
* `repsheet_redis_port <port>` - Redis port
* `repsheet_redis_timeout <n>` - Redis connection timeout (in milliseconds)
* `repsheet_user_cookie <name>` - Cookie that holds actor value

Here's a simple example NGINX config:

```
events {
  worker_connections  1024;
}

http {
  repsheet on;
  repsheet_proxy_headers on;

  repsheet_redis_host localhost;
  repsheet_redis_port 6379;
  repsheet_redis_timeout 5;

  repsheet_ip_lookup on;
  repsheet_user_lookup on;
  repsheet_user_cookie "user"

  server {
    location / {

    }
  }
}
```
