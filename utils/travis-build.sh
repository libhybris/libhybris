#!/bin/sh -ex

run_first_stage() {
	docker pull ubuntu:16.04
	docker run -i -t -v $PWD:/libhybris ubuntu:16.04 /libhybris/utils/travis-build.sh --second-stage
}

run_second_stage() {
	# We're now inside the docker container and need to install all
	# necassary build dependencies first.
	apt-get update -qq
	apt-get install -qq -y \
	  autoconf \
	  automake \
	  autopoint \
	  autotools-dev \
	  android-headers \
	  build-essential \
	  git \
	  pkg-config \
	  libgles2-mesa-dev \
	  libwayland-dev \
	  libtool \
	  autoconf-archive

	cd /libhybris/hybris

	# Cleanup everything
	git clean -fdx . || true
	git reset --hard || true

	./autogen.sh
	./configure \
		--enable-arch=x86 \
		--enable-wayland \
		--enable-experimental \
		--with-android-headers=/usr/include/android

	make -j10
}

if [ "$1" = "" ]; then
	run_first_stage
elif [ "$1" = "--second-stage" ]; then
	run_second_stage
fi
