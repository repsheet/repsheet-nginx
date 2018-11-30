#!/bin/sh

# Install most current version of redis
git clone git://github.com/antirez/redis
cd redis
make
sudo make install

# Install Redis Module
git clone git://github.com/repsheet/redis_module
cd redis_module
make
redis-server --loadmodule ./module.so &
cd ..

# Compile and run tests
script/bootstrap
bundle install
bundle exec rake
