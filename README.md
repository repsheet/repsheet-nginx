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

At the moment only source code is provided. Debian and RedHat packages
will come later. To install you will need automake, autoconf, libtool,
hiredis, and pcre. To use the library you will need a Redis server
available. Once you have the dependencies installed, you can run the
following to build and install librepsheet:

```sh
$ ./autogen.sh
$ ./configure
$ make
$ make install
```

To verify that the installation completed successfully, you can check
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
milliseconds.

To operate on an actor the following functions are provided:

```
mark_actor - Adds the actor to the Repsheet
blacklist_actor - Adds the actor to the Repsheet blacklist
whitelist_actor - Adds the actor to the Repsheet whitelist
is_on_repsheet - Checks if the actor is on the Repsheet
is_blacklisted - Checks if the actor is on the blacklist
is_whitelisted - Checks if the actor is on the whitelist
is_historical_offender - Checks if the actor has been previously blacklisted
```

All functions above take the Redis context `redisContext *context` and
the ip address of the actor `const char *actor` as their
arguments. For example, if you wanted to place an actor on the
blacklist and check later that they have been blacklisted you can do
the following:

```c
redisContext *context = get_redis_context("localhost", 6379, 5);

const char *address = "127.0.0.1";

blacklist_actor(context, address);

/* time passes... */

if (is_blacklisted(context, address)) {
  printf("Actor %s is blacklisted\n", address);
}

redisFree(context);
```

To get a full list of the functions available in librepsheet along
with examples you can visit the [project page](http://repsheet.github.io/librepsheet/)

## Contributing

Contributions to librepsheet are always welcome! Before you hit the
pull request button there are a few things to consider. This project
runs at 100% code coverage (via lcov) and a clean coverity
report. This might seem like a lot of additional work, but with a C
library meant for a wide variety of use cases it is important. Pull
requests that don't keep the coverage at 100% will be rejected. If you
are adding new functions/functionality make sure to only add one
function or one functional group per pull request. This keeps the
review time to a minimum and the commits focused. The tests for this
project run on travis ci. A hook on the project will make sure the
tests are passing before pull requests are merged. If the tests are
not passing the pull request will be rejected. More info about your
setup as a developer can be found below.

## Developer Setup

If you wish to contribute to librepsheet you will need some additional
dependencies. The check, lcov, and doxygen libraries are required to
complete a full build of librepsheet. The test suite is built using
the check library. To run the tests simply run `make check`. The
documentation is created via doxygen. Generating new documentation
pages is done via `make doc`. Code coverage is done via lcov. To see a
code coverage report just run `script/coverage`. This will perform a
clean and full test run and generate the HTML coverage report. The
last step in `script/coverage` is to open the chromium browser on
Linux. If you don't have that installed or aren't running Linux this
step may fail.
