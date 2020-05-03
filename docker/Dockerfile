# this builds an image to use as a base for development
# docker build . -t cpp-build-base:0.1.0
FROM ubuntu:latest

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
	apt-get install -y build-essential git cmake autoconf libtool pkg-config lua5.3 lua5.3-dev python3 sqlite3 libsqlite3-dev
