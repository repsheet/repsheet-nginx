NGINX_VERSION=1.4.7

install_nginx () {
    if [ ! -d "vendor/nginx-$NGINX_VERSION" ]; then
        printf "$BLUE * $YELLOW Installing nginx $NGINX_VERSION$RESET "
        pushd vendor > /dev/null 2>&1
        curl -s -L -O "http://nginx.org/download/nginx-$NGINX_VERSION.tar.gz"
        tar xzf "nginx-$NGINX_VERSION.tar.gz"
        printf "."
        pushd "nginx-$NGINX_VERSION" > /dev/null 2>&1
        ./configure                           \
            --with-debug                      \
            --prefix=$(pwd)/../../build/nginx \
            --conf-path=conf/nginx.conf       \
            --error-log-path=logs/error.log   \
            --http-log-path=logs/access.log   \
            --add-module=../../ > install.log 2>&1
        printf "."
        make >> install.log 2>&1
        printf "."
        make install >> install.log 2>&1
        popd > /dev/null 2>&1
        popd > /dev/null 2>&1
        ln -sf $(pwd)/nginx.conf $(pwd)/build/nginx/conf/nginx.conf
        printf "."
        printf " $GREEN [Complete] $RESET\n"
    else
        printf "$BLUE * $GREEN nginx already installed $RESET\n"
    fi
}
