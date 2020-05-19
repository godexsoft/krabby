# to build cpp-build-base check docker/Dockerfile
FROM cpp-build-base:0.1.0 AS build
WORKDIR /src
COPY CMakeLists.txt ./
COPY include/ ./include 
COPY lib/ ./lib
COPY src/ ./src
RUN mkdir build && cd build && cmake -DENABLE_LOG=ON -DCRAB_TLS=ON .. && make

FROM ubuntu:latest
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
	apt-get install -y lua5.3-dev libsqlite3-dev
WORKDIR /opt/krabby
COPY --from=build /src/build/krabby ./
EXPOSE 8080 8080
STOPSIGNAL SIGINT
ENTRYPOINT ["./krabby", "-l", "/data/"]
