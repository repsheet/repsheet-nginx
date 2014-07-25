librepsheet  [![Build Status](https://secure.travis-ci.org/repsheet/librepsheet.png)](http://travis-ci.org/repsheet/librepsheet?branch=master) [![Coverity Status](https://scan.coverity.com/projects/1749/badge.svg?flat=1)](https://scan.coverity.com/projects/1749)
===========

## Purpose

This is the core logic library for Repsheet. It provides the Redis
database operations, ModSecurity processing functions, and the forward
proxy parsing code. The idea is that this library will supply
everything needed to implement Repsheet in any program. The current
targets are the Apache and NGINX web servers, as well as the backend
processor for the current Repsheet system.

## Installation

You can install librepsheet using the Debian and RedHat packages
provided with the official release. To install librepsheet from
source you will need automake, autoconf, libtool, hiredis, and
pcre. To use the library you will need a Redis server available. Once
you have the dependencies installed, you can run the following to
build and install librepsheet:

```sh
$ ./autogen.sh
$ ./configure
$ make
$ make install
```

To verify that the installation completed successfully, check
pkg-config:

```sh
$ pkg-config --list-all | grep repsheet
repsheet                  repsheet - The Repsheet core logic library
```

## Example and Usage

librepsheet provides a connection function for Redis along with a
number of operations against Redis that are used in building
Repsheet. To get a connection use `get_redis_context`

```c
redisContext *context = get_redis_context("localhost", 6379, 5);
```

The arguments are `host`, `port`, and `timeout`. Timeout is in
milliseconds. The example below demonstrates how to check and actor
against the Repsheet database:

```c
redisContext *context = get_redis_context("localhost", 6379, 5);

int user_status = actor_status(context, "cookie_value", USER);
int ip_status = actor_status(context, "127.0.0.1", IP);

if (user_status == DISCONNECTED || ip_status == DISCONNECTED) {
  printf("Something went wrong.\n");
}

if (user_status == WHITELISTED || ip_status == WHITELISTED) {
  printf("Nothing to see here, move along.\n");
}

if (user_status == BLACKLISTED || ip_status == BLACKLISTED) {
  printf("None shall pass!\n");
}

if (user_status == MARKED || ip_status == MARKED) {
  printf("There's something funny about this one.\n");
}

redisFree(context);
```

To view all of the functions provided by librepsheet you can visit the
[documentation page](http://repsheet.github.io/librepsheet/docs/librepsheet_8c.html).
