#!/bin/zsh

set -e
BUILD_DIR="build"
LLVM_PATH="/usr/local/opt/llvm"
SDK_PATH=$(xcrun --show-sdk-path)


if [[ "$1" == "clean" ]]; then
	rm -rf $BUILD_DIR
	shift
fi


mkdir -p $BUILD_DIR


if [[ "$1" == "run" ]]; then
	shift
	CMAKE_ARGS=""
	if [[ $# -gt 0 ]]; then
		CMAKE_ARGS="-DARGS=\"$*\""
	fi

	cmake -B $BUILD_DIR -S . \
		-DCMAKE_CXX_COMPILER="$LLVM_PATH/bin/clang++" \
		-DCMAKE_CXX_FLAGS="-std=c++23 -nostdinc++ -isystem /usr/local/opt/llvm/include/c++/v1 -I/usr/local/opt/llvm/include -stdlib=libc++ -isysroot $SDK_PATH" \
		-DCMAKE_EXE_LINKER_FLAGS="-nostdlib++ -L/usr/local/opt/llvm/lib/c++ -lc++ -L$LLVM_PATH/lib -L$LLVM_PATH/lib/unwind -lunwind -Wl,-rpath,$LLVM_PATH/lib" \
		$CMAKE_ARGS
	export VERBOSE=1
	cmake --build $BUILD_DIR -j$(sysctl -n hw.logicalcpu)

	cmake --build $BUILD_DIR --target run
	exit 0
fi


cmake -B $BUILD_DIR -S . \
	-DCMAKE_CXX_COMPILER="$LLVM_PATH/bin/clang++" \
	-DCMAKE_CXX_FLAGS="-std=c++23 -nostdinc++ -isystem /usr/local/opt/llvm/include/c++/v1 -I/usr/local/opt/llvm/include -stdlib=libc++ -isysroot $SDK_PATH" \
	-DCMAKE_EXE_LINKER_FLAGS="-nostdlib++ -L/usr/local/opt/llvm/lib/c++ -lc++ -L$LLVM_PATH/lib -L$LLVM_PATH/lib/unwind -lunwind -Wl,-rpath,$LLVM_PATH/lib"

export VERBOSE=1

cmake --build $BUILD_DIR -j$(sysctl -n hw.logicalcpu)
