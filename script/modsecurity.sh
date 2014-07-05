MODSECURITY_VERSION=2.8.0

install_mod_security () {
    if [ ! -d "vendor/modsecurity-apache_$MODSECURITY_VERSION" ]; then
        pushd vendor > /dev/null 2>&1
        curl -s -O https://www.modsecurity.org/tarball/$MODSECURITY_VERSION/modsecurity-$MODSECURITY_VERSION.tar.gz
        tar xzf modsecurity-$MODSECURITY_VERSION.tar.gz
        pushd modsecurity-$MODSECURITY_VERSION > /dev/null 2>&1
	mkdir $(pwd)/../../build/nginx/lib
        ./configure --enable-standalone-module     \
	    --prefix=$(pwd)/../../build/nginx      \
            --exec-prefix=$(pwd)/../../build/nginx
        make
        make install
        popd > /dev/null 2>&1
        popd > /dev/null 2>&1
    fi
}

configure_modsecurity () {
    if [[ ! -d "build/nginx/conf/modsecurity" ]] ; then
        cp -r modsecurity build/nginx/conf/
    fi
}
