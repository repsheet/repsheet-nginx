librepsheet  [![Build Status](https://secure.travis-ci.org/repsheet/librepsheet.png)](http://travis-ci.org/repsheet/librepsheet?branch=master) [![Coverity Status](https://scan.coverity.com/projects/1749/badge.svg?flat=1)](https://scan.coverity.com/projects/1749)
===========

## Purpose

This is the core logic library for Repsheet. It provides the Redis
database operations, and X-Forwarded-For parsing code. This library
will supply everything needed to implement Repsheet in any
program. The current targets are the Apache and NGINX web servers.

## Installation

You can install librepsheet using the Debian and RedHat packages
provided with the official release. To install librepsheet from source
you will need automake, autoconf, and libtool. The check library is
necessary if you want to run the test suite. To use the library you
will need a Redis server available. Once you have the dependencies
installed, you can run the following to build and install librepsheet:

```sh
$ ./autogen.sh
$ ./configure
$ make
$ make check # all tests should pass
$ <sudo> make install
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
Repsheet. To get a connection use `repsheet_connect`

```c
redisContext *context = repsheet_connect("localhost", 6379, 5, 10);
```

The arguments are `host`, `port`, `connect_timeout`, and
`read_timeout`. Each timeout argument is considered to be in
milliseconds. The example below demonstrates how to check and actor
against the Repsheet database:

```c
#include <stdio.h>
#include <repsheet.h>

int main(void)
{
  redisContext *context = repsheet_connect("localhost", 6379, 5, 10);
  char reason[MAX_REASON_LENGTH];
  int status = actor_status(context, "127.0.0.1", IP, reason);

  if (status == DISCONNECTED) {
    printf("Redis connection unavailable\n");
  } else if (status == WHITELISTED) {
    printf("IP is whitelisted: %s\n", reason);
  } else if (status == BLACKLISTED) {
    printf("IP is blacklisted: %s\n", reason);
  } else if (status == MARKED) {
    printf("IP is marked: %s\n", reason);
  } else if (status == LIBREPSHEET_OK) {
    printf("IP is considered ok\n");
  }

  redisFree(context);
  return 0;
}
```

To view all of the functions provided by librepsheet you can visit the
[documentation page](http://repsheet.github.io/librepsheet/docs/librepsheet_8c.html).
