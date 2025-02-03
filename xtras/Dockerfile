FROM ubuntu:jammy
ARG DEBIAN_FRONTEND=noninteractive
ARG CACHEBUST=1
RUN apt update -qq && apt upgrade -y -qq && \
     apt autoremove --purge -y -qq && \
     apt clean -y -qq && \
     apt install -y -qq zip git \
     	 debhelper debmake fakeroot libncursesw5-dev \
     	 autoconf make checkinstall \
     	 dh-autoreconf dh-make dh-strip-nondeterminism && \
     apt autoremove --purge -y -qq && \
     apt clean -y -qq

RUN useradd -ms /bin/bash paag
USER paag
