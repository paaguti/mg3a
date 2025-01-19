#!/bin/bash

cd $(dirname $0)


if [ -d src ]; then
  cd src; git clean -dfx; git pull; cd ..
else
  git clone https://github.com/paaguti/mg3a src
fi

SEGPRG=$(printf "/^UPDATE=/s/UPDATE=./UPDATE=%s/g" $UPDATE)

IMAGE=jammy
PRG=/usr/local/bin/mkmg
MSG=1
# while [ -n "$(docker ps -aq)" ]; do
#     [ $MSG -eq 1 ] && echo "Waiting for other compilation to end..."
#     sleep 1
#     MSG=0
# done

[ -d $(pwd)/debs ] || mkdir -p $(pwd)/debs
[ -z "$(docker image ls debmaker:${IMAGE} -aq)" ] && docker -D build -t debmaker:${IMAGE} . -f Dockerfile
echo "Compiling for ${IMAGE}..."

docker -D run -it \
       --mount "type=bind,source=$(pwd)/bin,target=/usr/local/bin" \
       --mount "type=bind,source=$(pwd)/src,target=/home/paag/src" \
       --mount "type=bind,source=$(pwd)/debs,target=/home/paag/debs" \
       --entrypoint="" \
       --user=$UID:$GID \
       --workdir=/home/paag \
       --name mg3a-build debmaker:${IMAGE} ${PRG}

EXITED="$(docker ps -aq -f status=exited)"
DANGLING="$(docker image ls -q -f dangling=true)"

[ -n "$EXITED" ] && docker rm $EXITED
[ -n "$DANGLING" ] && docker rmi $EXITED

cd src; git clean -dfx
