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

## Examples

#### Connecting

librepsheet provides a connection function for Redis along with a
number of operations against Redis that are used in building
Repsheet. To get a connection use `repsheet_connect`. The arguments
are `host`, `port`, `connect_timeout`, and `read_timeout`. Each
timeout argument is considered to be in milliseconds. If the connect
or read timeouts are exceeded and error is set on the connection and
it is no longer usable.

```c
redisContext *context = repsheet_connect("localhost", 6379, 5, 10);
```

You can check the connection to ensure it is in tact before issuing a
command using the `check_connection` function:

```c
int status = check_connection(context);
```

If your connection is broken or has an error, you can attempt a
reconnect with the `repsheet_reconnect` function:

```c
int status = repsheet_reconnect(context);
```

#### Obtaining the status of an actor

Actors can be looked up by IP or by user. After establishing a connection, you can ask for the status of an actor using the `actor_status` function. You will need to provide a variable to hold the response of the lookup if the actor is found. The response contains the reason why the actor is present.

```c
char reason[MAX_REASON_LENGTH];
int status = actor_status(context, "1.1.1.1", IP, reason);
```

```c
char reason[MAX_REASON_LENGTH];
int status = actor_status(context, "d93c2e4e-40f5-4463-ac79-7a40e51eeae0", USER, reason);
```

The following are the possible status results for `actor_status`:

```c
BLACKLISTED
WHITELISTED
MARKED
LIBREPSHEET_OK
UNSUPPORTED
DISCONNECTED
```

Under normal circumstances you will only see the first four statuses. The last two are due to passing in an unsupported type to the lookup, or the connection has either been terminated or it has timed out. The result of `actor_status` should be used to determine if the actor should be able to continue what it is doing.

The following example shows a the code for a complete status check:

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

#### Processing the X-Forwarded-For Header

This library is commonly used inside of web servers. The most common
webservers (Apache and NGINX) do not have functions to process the
`X-Forwarded-For` header and extract the IP address of the actor. The
`remote_address` function can do this for you:

```c
char address[INET6_ADDRSTRLEN];
int result = remote_address("10.0.1.15", "1.1.1.1, 8.8.8.8, 2.2.2.2", address);
```

You must pass in a variable that is used to hold the result of the
processing. The possible return values are:

```c
BLACKLISTED
LIBREPSHEET_OK
NIL
```

If the processing is successful, `LIBREPSHEET_OK` is returned. If the
value is not a valid IPv4 or IPv6 address, `BLACKLISTED` is
returned. It is common to reject any request that does not have a
valid address in the XFF header because it is likely malicious in
nature. `NIL` is returned if improper values are provided to
`remote_address`.

## Redis Keyspace

Although librepsheet abstracts the Redis keyspace away, it is still
useful to understand how to compose values that librepsheet can lookup
if another system is writing values to Redis after processing attack
signals. The following scheme is used:

#### Blacklist

Key                                | Value
---------------------------------- | --------
`<ip>:repsheet:ip:blacklisted`     | `<reason>`
`<user>:repsheet:user:blacklisted` | `<reason>`
`<cidr>:repsheet:cidr:blacklisted` | `<reason>`
`repsheet:cidr:blacklisted`        | `<cidr>`

#### Whitelist

Key                                | Value
---------------------------------- | --------
`<ip>:repsheet:ip:whitelisted`     | `<reason>`
`<user>:repsheet:user:whitelisted` | `<reason>`
`<cidr>:repsheet:cidr:whitelisted` | `<reason>`
`repsheet:cidr:whitelisted`        | `<cidr>`

#### Marked

Key                           | Value
----------------------------- | --------
`<ip>:repsheet:ip:marked`     | `<reason>`
`<user>:repsheet:user:marked` | `<reason>`
`<cidr>:repsheet:cidr:marked` | `<reason>`
`repsheet:cidr:marked`        | `<cidr>`

To view all of the functions provided by librepsheet you can visit the
[documentation page](http://repsheet.github.io/librepsheet/docs/librepsheet_8c.html). To
see a complete example of librepsheet used in a webserver module, see
the [Apache](https://github.com/repsheet/repsheet-apache) or
[NGINX](https://github.com/repsheet/repsheet-nginx) modules.
