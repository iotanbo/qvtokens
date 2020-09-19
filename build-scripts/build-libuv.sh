#!/usr/bin/env bash

# Build a specific version of [libuv] 
# that is current project's dependency

if [ -d "../libuv" ]; then
	echo "=> Looks like [libuv] directory already exists, not cloning from github.";
else
    echo "=> Cloning [libuv] from github...";
    # The latest version on master branch:
    git clone --depth 1 https://github.com/libuv/libuv.git ../libuv
    # Or another libuv version:
    # - git clone --branch v1.39.0 --depth 1 https://github.com/libuv/libuv.git ../libuv
fi

cd ../libuv
./autogen.sh
./configure --enable-static --disable-shared
make -j3
cd ../qvtokens
