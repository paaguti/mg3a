#!/usr/bin/env bash

docker -D build -t fedora:builder . -f Dockerfile.f41
docker -D run -it \
       --mount "type=bind,source=$(pwd)/rpms,target=/root/rpmbuild/RPMS" \
       --mount "type=bind,source=$(pwd)/sources,target=/root/rpmbuild/SOURCES" \
       --mount "type=bind,source=$(pwd)/specs,target=/root/rpmbuild/SPECS" \
       --mount "type=bind,source=$(pwd)/bin,target=/usr/local/bin" \
       --name mg3a-f41 \
       --entrypoint="" fedora:builder /usr/local/bin/mkrpm
docker ps -aq | xargs docker rm
