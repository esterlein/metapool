#!/bin/zsh
set -e

TEMP_DIR=$(mktemp -d)
mkdir -p $TEMP_DIR/usr/lib/system
mkdir -p $TEMP_DIR/usr/local/lib

ln -sf /usr/local/opt/llvm/lib/libc++.1.dylib $TEMP_DIR/usr/lib/libc++.1.dylib
ln -sf /usr/local/opt/llvm/lib/libc++abi.dylib $TEMP_DIR/usr/lib/libc++abi.dylib

for LIB in libSystem.B.dylib libobjc.A.dylib liboah.dylib; do
  for DIR in /usr/lib /System/Library/Frameworks /System/Library/PrivateFrameworks; do
    find $DIR -name $LIB 2>/dev/null | head -1 | xargs -I{} ln -sf {} $TEMP_DIR/usr/lib/ 2>/dev/null || true
  done
done

for LIB in libdyld.dylib libsystem_c.dylib libsystem_malloc.dylib libsystem_info.dylib \
           libsystem_kernel.dylib libsystem_platform.dylib libsystem_pthread.dylib \
           libxpc.dylib libunwind.dylib libsystem_trace.dylib; do
  for DIR in /usr/lib/system /System/Library/Frameworks /System/Library/PrivateFrameworks; do
    find $DIR -name $LIB 2>/dev/null | head -1 | xargs -I{} ln -sf {} $TEMP_DIR/usr/lib/system/ 2>/dev/null || true
  done
done

for LIB in $(cat <<EOF
libc++.1.dylib libc++abi.dylib libobjc.A.dylib libSystem.B.dylib liboah.dylib
libsystem_c.dylib libsystem_malloc.dylib libdyld.dylib libsystem_info.dylib libxpc.dylib
libunwind.dylib libsystem_trace.dylib libsystem_kernel.dylib libsystem_platform.dylib
EOF
); do
  if [ ! -e "$TEMP_DIR/usr/lib/$LIB" ]; then
    touch $TEMP_DIR/usr/lib/$LIB
  fi
done

for LIB in $(cat <<EOF
libdyld.dylib libsystem_c.dylib libsystem_malloc.dylib libsystem_info.dylib
libxpc.dylib libunwind.dylib libsystem_trace.dylib libsystem_kernel.dylib
EOF
); do
  if [ ! -e "$TEMP_DIR/usr/lib/system/$LIB" ]; then
    touch $TEMP_DIR/usr/lib/system/$LIB
  fi
done

DYLD_LIBRARY_PATH=/usr/local/opt/llvm/lib:/usr/local/opt/llvm/lib/c++ \
DYLD_FALLBACK_LIBRARY_PATH=$TEMP_DIR/usr/lib:$TEMP_DIR/usr/lib/system \
PPROF_BINARY_PATH=$TEMP_DIR \
pprof --text $1 $2

rm -rf $TEMP_DIR
