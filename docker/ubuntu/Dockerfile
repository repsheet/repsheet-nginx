FROM ubuntu:latest

MAINTAINER aaron@aaronbedra.com

RUN apt-get -y update
RUN apt-get -y upgrade
run apt-get -y install git libtool autoconf automake make gcc curl libcurl4-openssl-dev libpcre3-dev zlib1g-dev libhiredis0.13 libhiredis-dev

WORKDIR /build

COPY build.sh .

RUN bash build.sh

RUN tail -f /dev/null
