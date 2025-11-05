#!/usr/bin/env bash

set -e
build_dir="build"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"


if [[ "$(uname)" == "Darwin" ]]; then
	platform="macos"
	sdk_path=$(xcrun --show-sdk-path)
	llvm_path="/usr/local/opt/llvm" # adjust as needed
	cpu_count=$(sysctl -n hw.logicalcpu)
else
	platform="linux"
	sdk_path=""
	cpu_count=$(nproc)
fi

if [[ -z "$compiler" ]]; then
	if command -v clang++ > /dev/null 2>&1; then
		compiler="clang++"
	elif command -v g++ > /dev/null 2>&1; then
		compiler="g++"
	else
		echo "no suitable compiler found (clang++ or g++)" >&2
		exit 1
	fi
fi

cxx="$compiler"

clean_build_dir() {
	rm -rf "$build_dir"
	mkdir -p "$build_dir"
}

configure_and_build() {
	extra_args="$1"

	cxx_flags="-std=c++23"
	linker_flags=""
	compiler_path="$cxx"

	if [[ "$platform" == "macos" && "$cxx" == "clang++" ]]; then
		compiler_path="$llvm_path/bin/clang++"
		cxx_flags+=" -nostdinc++ -isystem $llvm_path/include/c++/v1 -I$llvm_path/include -stdlib=libc++ -isysroot $sdk_path"
		linker_flags="-nostdlib++ -L$llvm_path/lib/c++ -lc++ -L$llvm_path/lib -L$llvm_path/lib/unwind -lunwind -Wl,-rpath,$llvm_path/lib"
	fi

	cmake -B "$build_dir" -S . \
		-DCMAKE_CXX_COMPILER="$compiler_path" \
		-DCMAKE_CXX_FLAGS="$cxx_flags" \
		-DCMAKE_EXE_LINKER_FLAGS="$linker_flags" \
		$extra_args

	export VERBOSE=1
	cmake --build "$build_dir" -j"$cpu_count"

	if [[ -f "$script_dir/$build_dir/compile_commands.json" ]]; then
		ln -sf "$script_dir/$build_dir/compile_commands.json" "$script_dir/compile_commands.json"
		ln -sf "$script_dir/$build_dir/compile_commands.json" "$script_dir/../compile_commands.json"
	fi

	mkdir -p "$build_dir/trace"
}

clean=0

if [[ "$1" == "clean" ]]; then
	clean=1
	shift
fi

[[ "$clean" -eq 1 ]] && clean_build_dir

if [[ "$1" == "run" ]]; then
	shift
	args="$*"
	cmake_args=""
	[[ -n "$args" ]] && cmake_args="-DARGS=$args"
	configure_and_build "$cmake_args"
	cmake --build "$build_dir" --target run
	exit 0
fi

configure_and_build ""
