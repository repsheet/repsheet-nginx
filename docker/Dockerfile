FROM centos

MAINTAINER pprokopowicz@groupon.com

# basic compilation, git, etc
RUN yum install -y git libtool autoconf automake make gcc check check-devel curl curl-devel wget bash vim gpg


# update yum to know about redis and then get redis
RUN mkdir /code
WORKDIR /code

RUN wget -r --no-parent -A "epel-release-*.rpm" http://dl.fedoraproject.org/pub/epel/7/x86_64/e/
RUN rpm -Uvh dl.fedoraproject.org/pub/epel/7/x86_64/e/epel-release-*.rpm
RUN yum install -y redis

ENV LD_LIBRARY_PATH /usr/local/lib

#download source that needs to be built

RUN git clone https://github.com/redis/hiredis.git
RUN git clone https://github.com/repsheet/librepsheet.git
RUN git clone https://github.com/repsheet/repsheet-nginx.git

WORKDIR /code/hiredis
RUN make
RUN make install

#set up ruby
RUN gpg2 --keyserver hkp://keys.gnupg.net --recv-keys 409B6B1796C275462A1703113804BB82D39DC0E3
RUN /bin/bash -l -c "curl -L get.rvm.io | bash -s stable --ruby"

#build and install librepsheet
WORKDIR /code/librepsheet
RUN ./autogen.sh
RUN ./configure
RUN make
RUN /bin/bash -c "redis-server&  make check"
RUN make install

#build nginx module
WORKDIR /code/repsheet-nginx
RUN /bin/bash -l -c "gem install bundle"
RUN /bin/bash -l -c "bundle install"
RUN /bin/bash -l -c "./script/bootstrap"
RUN /bin/bash -l -c "rake nginx:compile"
RUN /bin/bash -l -c "redis-server& rake" 
