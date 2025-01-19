#!/usr/bin/env bash

[ -d rpms ] || mkdir rpms
if [ -d src ]; then
  cd src; git clean -dfx; git pull; cd ..
else
  git clone https://github.com/paaguti/mg3a src
fi
docker -D build -t fedora:builder . -f Dockerfile.f41
docker -D run -it \
       --mount "type=bind,source=$(pwd)/rpms,target=/root/rpmbuild/RPMS" \
       --mount "type=bind,source=$(pwd)/specs,target=/root/rpmbuild/SPECS" \
       --mount "type=bind,source=$(pwd)/bin,target=/usr/local/bin" \
       --mount "type=bind,source=$(pwd)/src,target=/root/rpmbuild/SOURCES" \
       --name mg3a-f41 \
       --entrypoint="" fedora:builder /usr/local/bin/mkrpm
docker ps -aq | xargs docker rm
