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

# Build and run test suite
./autogen.sh
./configure
make
make check
