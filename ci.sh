#!/bin/sh

# Install a supported version of hiredis
curl -O -L https://github.com/redis/hiredis/archive/v0.13.1.tar.gz
tar xzf v0.13.1.tar.gz
cd hiredis-0.13.1
./configure
make
sudo make install
sudo ldconfig
cd ..

# Install librepsheet
git clone git://github.com/repsheet/librepsheet
cd librepsheet
./autogen.sh
./configure
make
sudo make install
sudo ldconfig
pkg-config --list-all
cd ..

# Compile and run tests
script/bootstrap
bundle install
bundle exec rake
