# Repsheet NGINX [![Build Status](https://secure.travis-ci.org/repsheet/repsheet-nginx.png)](http://travis-ci.org/repsheet/repsheet-nginx?branch=master)

## How does it work?

Repsheet works by inspecting requests as they come into the web server
and checking the requesting actor's status in Redis. When a request
comes in, the IP is checked to see if it has been flagged by
Repsheet. If the actor has been flagged, that information is logged
and the header `X-Repsheet: true` will be added to the downstream
request to let the application know that the actor is suspected of
malicious activity. If the actor has been blacklisted, Repsheet
instructs NGINX to return a 403.

An actor can be defined by either an IP address or a cookie value. By
default Repsheet looks at the IP address of the requesting actor by
using the directly connected IP address provided by NGINX or by
examining the `X-Forwarded-For` header if enabled using the
`repsheet_proxy_headers` directive. If the `repsheet_user_cookie`
directive is given a value, it will test the value of that cookie in
each request to see if that cookie has been blacklisted.

## Dependencies

This module requires [hiredis](https://github.com/redis/hiredis) and
[librepsheet](https://github.com/repsheet/librepsheet) >= 3.0.0 for
compilation and [Redis](http://redis.io) for its runtime.

#### Installation

Since NGINX doesn't have shared module support, this module will need
to be accessible during the compilation of NGINX itself. The `scripts`
folder has some examples on how to add the module to your NGINX
installation.

To activate and configure Repsheet you will need to set some
directives. The following list explains what each directive is and
what is does. At the moment, the Repsheet directives all live under
the main configuration section of `nginx.conf`.

* `repsheet <on|off>` - Determines if Repsheet will do any processing
* `repsheet_ip_lookup <on|off>` - Determines if Repsheet will lookup actors by IP address
* `repsheet_user_lookup <on|off>` - Determines if Repsheet will lookup actors by user cookie
* `repsheet_proxy_headers <on|off>` - Determines if Repsheet will use the X-Forwarded-For header

* `repsheet_redis_host <host>` - Sets the host for the Redis connection
* `repsheet_redis_port <port>` - Sets the port for the Redis connection
* `repsheet_redis_timeout <n>` - Sets the Redis connection timeout (in milliseconds)

* `repsheet_user_cookie <name>` - Sets the cookie that holds the user value to be examined

Here's a simple example NGINX config:

```
events {
  worker_connections  1024;
}

http {
  repsheet on;
  repsheet_ip_lookup on;
  repsheet_user_lookup on;
  repsheet_proxy_headers on;

  repsheet_redis_host localhost;
  repsheet_redis_port 6379;
  repsheet_redis_timeout 5;

  repsheet_user_cookie "user"

  server {
    listen 8888;
    location / {

    }
  }
}
```

## Running the Integration Tests

This project comes with a basic set of integration tests to ensure
that things are working. If you want to run the tests, you will need
to have [Ruby](http://www.ruby-lang.org/en/),
[RubyGems](http://rubygems.org/), and [Bundler](http://bundler.io/)
installed. In order to run the integration tests, use the following
commands:

```sh
bundle install
script/bootstrap
rake nginx:compile
rake
```

The `script/bootstrap` task will take some time. It downloads and
compiles NGINX, and then configures everything to work
together. Running `rake` launches some curl based tests that hit the
site and exercise Repsheet, then test that everything is working as
expected.

## Feature Differences from the Apache Version

The concept of what Repsheet should do is constantly evolving. There
are some differences between the NGINX and Apache modules and it is
important to make them as clear as possible.

#### The Unsupported

* This module does not have the recorder feature that the Apache
  version does. It will be implemented, but doesn't currently exist.

* This module does not support ModSecurity. The development of this
  support is blocked due to an incomplete implementation of
  ModSecurity for NGINX. The issue tracking this is
  https://github.com/SpiderLabs/ModSecurity/issues/660. Once this is
  resolved or the feature can be implemented in another way it will
  be.

* This module does not currently support GeoIP based lookups. There is
  nothing blocking this from happening, it just has not been
  implemented yet.
