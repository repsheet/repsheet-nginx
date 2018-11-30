# Repsheet NGINX [![Build Status](https://secure.travis-ci.org/repsheet/repsheet-nginx.png)](http://travis-ci.org/repsheet/repsheet-nginx?branch=master)

## How does it work?

Repsheet works by inspecting requests as they come into the web server
and checking the requesting actor's status in a cache. When a request
comes in, the status of the actor is used to determine the required
action. If the actor has been flagged, that information is added and
the header `X-Repsheet: <reason>`. If the actor has been blacklisted,
Repsheet instructs NGINX to return a 403.

An actor can be defined by either an IP address or a cookie value. By
default Repsheet looks at the IP address of the requesting actor
using the directly connected IP address provided by NGINX or by
examining the `X-Forwarded-For` header if enabled using the
`repsheet_proxy_headers` directive.

## Dependencies

* [hiredis](https://github.com/redis/hiredis) 0.11 or greater
* [Redis](http://redis.io) 4.0 or greater
* [Repsheet Redis Module](https://github.com/repsheet/redis_module)

#### Installation

You can install this module the traditional way by compiling it in or
by compiling it as a shared module. To activate and configure
Repsheet you will need to set some directives. The following list
explains what each directive is and what is does.

* `repsheet <on|off>` - Determines if Repsheet will do any processing

* `repsheet_redis_host <host>` - Sets the host for the Redis connection
* `repsheet_redis_port <port>` - Sets the port for the Redis connection
* `repsheet_redis_connection_timeout <n>` - Sets the Redis connection timeout (in milliseconds)
* `repsheet_redis_read_timeout <n>` - Sets the Redis request timeout (in milliseconds)

* `repsheet_proxy_headers <on|off>` - Determines if Repsheet will use the X-Forwarded-For header
* `repsheet_proxy_headers_header <header>` - Sets an alternate header to use for the source ip
* `repsheet_proxy_fallback <on|off>` - Uses X-Forwarded-For as a fallback if `proxy_headers_header` fails to find a valid address

Here's a simple example NGINX config:

```
events {
  worker_connections  1024;
}

http {
  repsheet on;
  repsheet_redis_host localhost;
  repsheet_redis_port 6379;
  repsheet_redis_connection_timeout 5;
  repsheet_redis_read_timeout 10;
  repsheet_proxy_headers on;
  repsheet_proxy_headers_header "True-Client-IP";
  repsheet_proxy_fallback on;

  proxy_set_header X-Repsheet $repsheet;

  server {
    listen 8888;
    location / {

    }
  }
}
```

Notice the `proxy_set_header` directive. This is placed in the
configuration when applying marking. The module will populate the
`$repsheet` variable when a mark needs to be applied, and it will also
populate the `$repsheet_reason` variable with the reason for the
marking.

## Running the Integration Tests

This project comes with a basic set of integration tests to ensure
that things are working. If you want to run the tests, you will need
to have [Ruby](http://www.ruby-lang.org/en/),
[RubyGems](http://rubygems.org/), and [Bundler](http://bundler.io/)
installed. In order to run the integration tests, use the following
commands:

```sh
$ bundle install
$ script/bootstrap
$ rake
```

The `script/bootstrap` task will take some time. It downloads and
compiles NGINX, and then configures everything to work
together. Running `rake` launches some curl based tests that hit the
site and exercise Repsheet, then test that everything is working as
expected.

## Building for Linux with Docker

The file docker/Dockerfile works with docker to create a standalone
instance of CentOS Linux, install all the dependencies, and compile
and test librepsheet and repsheet-nginx.  Once you have installed
docker on your system simply run:

```
$ cd docker
$ docker build repsheet .
```

This builds the docker image defined in Dockerfile, which includes
creating a working instance of nginx with the repsheet module and
redis installed.
