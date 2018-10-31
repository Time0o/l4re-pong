#!/bin/bash

if [ $# -ne 1 ]; then
  echo "usage: $0 KPR_ROOT" 2>&1
  exit 1
fi

if [ ! -e "$1/obj/l4/x86" ]; then
  echo "$0: error: failed to find '$1/obj/l4/x86' directory" 2>&1
  exit 1
fi

cp mem/mem.cc "$1/src/l4/pkg/l4re-core/libc_backends/lib/l4re_mem"

cp -r pkg/* "$1/src/l4/pkg"

cp conf/modules.list "$1/src/l4/conf"

(
  cd "$1/src/l4/conf";
  ln -s "$(readlink -f ../pkg/fb-log/examples/fb-log-demo/fb_log_demo.lua)" .;
  ln -s "$(readlink -f ../pkg/fb-mux/examples/fb-mux-demo/fb_mux_demo.lua)" .;
  ln -s "$(readlink -f ../pkg/pong-client/examples/pong-client-demo/pong_client_demo.lua)" .
)

cd "$1/obj/l4/x86"

make -j 12

make qemu
