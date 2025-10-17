#!/bin/bash

set -e

function build
{
	BUILD_DIR="$(readlink -f "$1/build")"
	if [ -e "$1" ] ; then
		if [ -d "$BUILD_DIR" ] ; then
			echo "++ Cleaning $BUILD_DIR"
			rm -r "$BUILD_DIR"
		fi
	else
		echo "ERROR: $BUILD_DIR does not exist" >&2
		exit 1
	fi

	cmake -S "$1" -B "$BUILD_DIR"
	cmake --build "$BUILD_DIR" -- -j 8

	if [ "$RUN_TESTS" = "test" ] && [ -f "$1/test/CMakeLists.txt" ] ; then
		pushd "$BUILD_DIR"
		ctest
		popd
	fi
}

RUN_TESTS="nope"
if [ "$1" = "test" ] ; then
	RUN_TESTS="test"
fi

build io
build iou
build ipc++
build krb5
build log_filter

