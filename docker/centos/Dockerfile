FROM centos:latest

MAINTAINER aaron@aaronbedra.com

RUN yum -y install epel-release
RUN yum -y update
run yum -y install git libtool autoconf automake make gcc curl curl-devel pcre-devel zlib-devel hiredis hiredis-devel

WORKDIR /build

COPY build.sh .

RUN bash build.sh

RUN tail -f /dev/null
