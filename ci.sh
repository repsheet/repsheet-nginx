#!/bin/sh

# Install Redis Module
git clone git://github.com/repsheet/redis_module
cd redis_module
make
redis-cli module load ./module.so
cd ..

# Compile and run tests
script/bootstrap
bundle install
bundle exec rake
