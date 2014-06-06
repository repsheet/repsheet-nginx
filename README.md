# Repsheet NGINX [![Build Status](https://secure.travis-ci.org/repsheet/repsheet-nginx.png)](http://travis-ci.org/repsheet/repsheet-nginx?branch=master)

## How does it work?

Repsheet works by inspecting requests as they come into the web server
and storing that activity in Redis. When a request comes in, the IP is
checked to see if it has been flagged by Repsheet. If the actor has
been flagged, that information is logged and the header `X-Repsheet:
true` will be added to the downstream request to let the application
know that the actor is suspected of malicous activity. If the actor
has been blacklisted, Repsheet instructs NGINX to return a 403.`

## Dependencies

This module requires [hiredis](https://github.com/redis/hiredis) and
[librepsheet](https://github.com/repsheet/librepsheet) for compilation
and [Redis](http://redis.io) during execution.

#### Installation

Since NGINX doesn't have shared module support, this module will need
to be accessible during the compilation of NGINX itself. The `scripts`
folder has some examples on how to add the module to your NGINX
installation.

To activate and configure Repsheet you will need to set some
directives. The following list explains what each directive is and
what is does. There are main and location based configuration
options. For main configuration there are the following:

* `repsheet_redis_host <host>` - Sets the host for the Redis connection
* `repsheet_redis_port <port>` - Sets the port for the Redis connection
* `repsheet_redis_timeout <n>` - Sets the time (in milliseconds) before the attempt to connect to redis will timeout

The location configuration provides controls that can be tuned for
each individual location:

* `repsheet <on|off>` - Determines if Repsheet will do any processing
* `repsheet_proxy_headers <on|off>` - Determines if Repsheet will look for the X-Forwarded-For header to determine remote ip

Here's a simple example NGINX config:

```
events {
  worker_connections  1024;
}

http {
  repsheet_redis_host localhost;
  repsheet_redis_port 6379;
  repsheet_redis_timeout 5;

  server {
    listen 8888;
    location / {
      repsheet on;
      repsheet_proxy_headers on;
    }
  }
}
```

## Running the Integration Tests

This project comes with a basic set of integration tests to ensure
that things are working. If you want to run the tests, you will need
to have [Ruby](http://www.ruby-lang.org/en/) and
[RubyGems](http://rubygems.org/) installed. In order to run these, do
the following:

```sh
bundle install
script/bootstrap
rake
```

The `script/bootstrap` task will take some time. It downloads and
compiles NGINX and this module, and then configures everything to work
together. Running `rake` launches some curl based tests that hit the
site and exercise Repsheet, then test that everything is working as
expected.
