# Developing

## Developer Setup

In order to develop the module without interfering with other NGINX
installs, a completely isolated development environment is provided
with this module. A bootstrap script is provided to setup the
environment.

```sh
$ script/bootstrap
```

The `script/bootstrap` task will take some time. It downloads and
compiles NGINX, and then configures everything to work
together. Running this will give you a ready to test NGINX environment
linked to the `nginx.conf` file in the root of the project.

To use the rest of the provided automation you need to have
[Ruby](http://www.ruby-lang.org/en/),
[RubyGems](http://rubygems.org/), and [Bundler](http://bundler.io/)
installed. This allows you to build the module and run the
tests. Before executing any of the tasks, make sure to run `bundle`.

## Building the module

To compile the module simply run:

```sh
$ rake nginx:compile
```

This will rebuild NGINX along with the module.

## Running the Integration Tests

This project comes with a basic set of integration tests to ensure
that things are working. To run the integration tests, use the
following command:

```sh
rake
```

Running `rake` launches some curl based tests that hit NGINX and
exercise Repsheet, then test that everything is working as expected.
