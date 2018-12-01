#/usr/bin/env bash

set -o nounset
set -o errexit

REPSHEET_NGINX_VERSION=5.0.0
NGINX_VERSION=1.15.7

curl -s -L -O "https://github.com/repsheet/repsheet-nginx/archive/$REPSHEET_NGINX_VERSION.tar.gz"
tar xzf $REPSHEET_NGINX_VERSION.tar.gz

curl -s -L -O "http://nginx.org/download/nginx-$NGINX_VERSION.tar.gz"
tar xzf "nginx-$NGINX_VERSION.tar.gz"

pushd "nginx-$NGINX_VERSION"

./configure --add-dynamic-module=../repsheet-nginx-$REPSHEET_NGINX_VERSION
make modules

popd

echo "Build complete. The container is still running so the artifact can be copied"
