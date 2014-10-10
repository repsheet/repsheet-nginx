# Installation

NGINX does not support dynamically loadable modules. This means that
`repsheet-nginx` has to be compiled with NGINX. Adding modules to the
NGINX build requires one additional argument to the configuration
script.

```
--add-module=<path_to_module>
```

Where `path_to_module` is the location of this module. The script
attempts to locate the `config` file of the module, so the folder you
specify must contain that file. An example installation looks like
this:

```
$ git clone git://github.com/repsheet/repsheet-nginx
$ curl -s -L -O "http://nginx.org/download/nginx-1.6.2.tar.gz"
$ tar xzf nginx-1.6.2
$ cd nginx-1.6.2
$ ./configure --add-module=../repsheet-nginx
$ make
$ sudo make install
```

You might want to modify your NGINX install further. The compile
options can be found [here](http://wiki.nginx.org/InstallOptions). The
example above is the only thing you need to install the
`repsheet-nginx` module.

