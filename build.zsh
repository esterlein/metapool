#!/bin/zsh

set -e
BUILD_DIR="build"
LLVM_PATH="/usr/local/opt/llvm"
SDK_PATH=$(xcrun --show-sdk-path)

clean_build_dir() {
	rm -rf $BUILD_DIR
	mkdir -p $BUILD_DIR
}

configure_and_build() {
	local extra_args="$1"
	cmake -B $BUILD_DIR -S . \
		-DCMAKE_CXX_COMPILER="$LLVM_PATH/bin/clang++" \
		-DCMAKE_CXX_FLAGS="-std=c++23 -nostdinc++ -isystem $LLVM_PATH/include/c++/v1 -I$LLVM_PATH/include -stdlib=libc++ -isysroot $SDK_PATH" \
		-DCMAKE_EXE_LINKER_FLAGS="-nostdlib++ -L$LLVM_PATH/lib/c++ -lc++ -L$LLVM_PATH/lib -L$LLVM_PATH/lib/unwind -lunwind -Wl,-rpath,$LLVM_PATH/lib" \
		$extra_args

	export VERBOSE=1
	cmake --build $BUILD_DIR -j$(sysctl -n hw.logicalcpu)

	if [[ -f $BUILD_DIR/compile_commands.json ]]; then
		ln -sf $BUILD_DIR/compile_commands.json .
	fi
}

CLEAN=0
if [[ "$1" == "clean" ]]; then
	CLEAN=1
	shift
fi

[[ "$CLEAN" -eq 1 ]] && clean_build_dir

if [[ "$1" == "run" ]]; then
	shift
	args="$*"
	cmake_args=""
	[[ -n "$args" ]] && cmake_args="-DARGS=$args"
	configure_and_build "$cmake_args"
	cmake --build $BUILD_DIR --target run
	exit 0
fi

configure_and_build ""


# usage:

# clean              - clean the build directory
# (default)          - build normally
# run simple         - build and run with "simple" argument
# run intermediate   - build and run with "intermediate" argument
# run complex        - build and run with "complex" argument
