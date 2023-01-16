#!/bin/bash
set -eE

function usage () {
  printf "%s: build mg3a\nUsage:\n" $(basename $0)
  printf " %s [-r RELEASE] [-d distro] [-c] MGRELEASE\tto use specific RELEASE from github\n" $(basename $0)
  printf " %s [-r RELEASE] [-d distro] [-c] git\t\tto use HEAD from github\n\n" $(basename $0)
  printf "   -c          clean all packages before building\n" "$1"
  printf "   -d DISTRO:  distro  (default %s)\n" "$1"
  printf "   -r RELEASE: release (default %s)\n\n" "$2"
}

DISTRO=ubuntu
RELEASE=20.04
CLEAN=0
MGRELEASE=20211116

while getopts ":hcd:r:" opt; do
  case $opt in
    c)
      CLEAN=1
      ;;
    d)
      DISTRO=${OPTARG}
      ;;
    r)
      RELEASE=${OPTARG}
      ;;
    h|\?)
      usage ${DISTRO} ${RELEASE}
      exit
      ;;
  esac
done
shift $((OPTIND-1))

if [ $# -ne 1 ]; then
  usage ${DISTRO} ${RELEASE}
  exit
fi

MGRELEASE=$1
#
# Make sure that you have a -slim image for debian
#
if [ "${DISTRO}" == "debian" ]; then
  RELEASE="$(echo $RELEASE | sed 's/-slim//g' )-slim"
fi

IMAGE=${DISTRO}:${RELEASE}
echo "Using Image: ${IMAGE}"

#
# Debian docker images with -slim avoid many unnecesary dependencies and files
#
DEBDIR=$(echo ${DISTRO}-${RELEASE} | sed 's/-slim//g')

if [ ${CLEAN} -eq 1 ]; then
  [ -d  ${DEBDIR} ] && rm -vf ${DEBDIR}/mg*.deb
fi
[ -d  ${DEBDIR} ] || mkdir -v ${DEBDIR}

function cleanup () {
  if [ "$?" == "0" ]; then
    echo "Built successfully ($?): removing container"
    docker container rm --force build-z
  fi
  # [ $? -gt 0 ] && rm -rf ${DEBDIR}
  chown ${SUDO_USER}:${SUDO_USER} ${DEBDIR}
  chmod 0755 ${DEBDIR}
  chmod 0644 ${DEBDIR}/*
}

trap cleanup EXIT

function apt_install () {
  echo "DEBIAN_FRONTEND=noninteractive TZ=Europe/Madrid apt-get install -y $@"
}

docker container rm --force build-z || true

docker run -it -d --name build-z ${IMAGE} bash
#
# get the development libraries
#
docker exec build-z bash -c "DEBIAN_FRONTEND=noninteractive apt-get update"
#
# only upgrade the packages that strictly are needed in the build
#
# docker exec build-z bash -c "DEBIAN_FRONTEND=noninteractive apt-get upgrade -y"
#
docker exec build-z bash -c "$(apt_install build-essential)"
docker exec build-z bash -c "$(apt_install autotools-dev debhelper)"
docker exec build-z bash -c "$(apt_install libncurses-dev)"
if [ "${MGRELEASE}" != "git" ]; then
  docker exec build-z bash -c "$(apt_install wget unzip)"
  docker exec build-z bash -c "cd /root; wget https://github.com/paaguti/mg3a/archive/${MGRELEASE}.zip; unzip ${MGRELEASE}.zip"
  docker exec build-z bash -c "cd /root/mg3a-${MGRELEASE}; ./bootstrap.sh; debian/rules clean binary"
else
  docker exec build-z bash -c "$(apt_install git)"
  docker exec build-z bash -c "cd /root; git clone https://github.com/paaguti/mg3a.git"
  docker exec build-z bash -c "cd /root/mg3a; ./bootstrap.sh; debian/rules clean binary"
fi

for f in $(docker exec build-z bash -c "cd /root; ls *.deb")
do
  echo "build-z:/root/$f --> ${DEBDIR}"
  docker cp build-z:/root/$f ${DEBDIR}
done
