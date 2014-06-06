#!/bin/sh

git clone git://github.com/repsheet/librepsheet
cd librepsheet
./autogen.sh
./configure
make
sudo make install
sudo ldconfig
pkg-config --list-all
cd ..
script/bootstrap
bundle install
rake
