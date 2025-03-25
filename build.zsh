#!/bin/zsh

set -e
BUILD_DIR="build"
LLVM_PATH="/usr/local/opt/llvm"
SDK_PATH=$(xcrun --show-sdk-path)


CLEAN=0
if [[ "$1" == "clean" ]]; then
	CLEAN=1
	shift
fi

if [[ "$CLEAN" -eq 1 ]]; then
	rm -rf $BUILD_DIR
	mkdir -p $BUILD_DIR
fi


if [[ "$1" == "run" ]]; then
	shift
	CMAKE_ARGS=""
	if [[ $# -gt 0 ]]; then
		CMAKE_ARGS="-DARGS=$*"
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


if [[ "$1" == "profile" ]]; then
	shift
	CMAKE_ARGS=""
	if [[ $# -gt 0 && "$1" != "analyze" && "$1" != "analyze-web" ]]; then
		CMAKE_ARGS="-DARGS=$*"
	fi
	
	cmake -B $BUILD_DIR -S . \
		-DCMAKE_CXX_COMPILER="$LLVM_PATH/bin/clang++" \
		-DCMAKE_CXX_FLAGS="-std=c++23 -nostdinc++ -isystem /usr/local/opt/llvm/include/c++/v1 -I/usr/local/opt/llvm/include -stdlib=libc++ -isysroot $SDK_PATH -g" \
		-DCMAKE_EXE_LINKER_FLAGS="-nostdlib++ -L/usr/local/opt/llvm/lib/c++ -lc++ -L$LLVM_PATH/lib -L$LLVM_PATH/lib/unwind -lunwind -Wl,-rpath,$LLVM_PATH/lib" \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		$CMAKE_ARGS
	
	export VERBOSE=1
	cmake --build $BUILD_DIR -j$(sysctl -n hw.logicalcpu)
	cmake --build $BUILD_DIR --target profile
	
	if [[ "$1" == "analyze" ]]; then
		cmake --build $BUILD_DIR --target analyze
		exit 0
	elif [[ "$1" == "analyze-web" ]]; then
		cmake --build $BUILD_DIR --target analyze-web
		exit 0
	fi
	
	exit 0
fi


if [[ "$1" == "analyze" ]]; then
	cmake --build $BUILD_DIR --target analyze
	exit 0
fi

if [[ "$1" == "analyze-web" ]]; then
	cmake --build $BUILD_DIR --target analyze-web
	exit 0
fi


cmake -B $BUILD_DIR -S . \
	-DCMAKE_CXX_COMPILER="$LLVM_PATH/bin/clang++" \
	-DCMAKE_CXX_FLAGS="-std=c++23 -nostdinc++ -isystem /usr/local/opt/llvm/include/c++/v1 -I/usr/local/opt/llvm/include -stdlib=libc++ -isysroot $SDK_PATH" \
	-DCMAKE_EXE_LINKER_FLAGS="-nostdlib++ -L/usr/local/opt/llvm/lib/c++ -lc++ -L$LLVM_PATH/lib -L$LLVM_PATH/lib/unwind -lunwind -Wl,-rpath,$LLVM_PATH/lib"
export VERBOSE=1
cmake --build $BUILD_DIR -j$(sysctl -n hw.logicalcpu)
