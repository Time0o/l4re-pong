# TU Dresden _Microkernel Operating Systems Lab_ solution

This is my solution to the _Microkernel Operating Systems Lab_ at TU
Dresden. It mainly consists of implementations of basic memory management
functions and serval client/server programs for the
[L4Re](https://github.com/kernkonzept/l4re-core) microkernel operating system
which together provide enough basic functionality to enable a user to play a
game of pong via keyboard-input.

The repsitory is structured as follows:

* `mem/mem.cc` contains implementations of the libc `malloc`, `calloc`,
  `realloc` and `free` functions for L4Re.

* `pkg/fb-log` implements a "framebuffer log" server that allows its client to
  write log output to the system's physical framebuffer.

* `pkg/fb-mux` implements a "framebuffer multiplexer" server which allows
  several clients to concurrently access the physical framebuffer.

* `pkg/keyboard-drv` implements a "keyboard driver" server which allows its
  clients to listen for user keyboard input.

* `pkg/pong-client` implements the main client controlling the flow of the pong
  session, it talks to an already present pong server which is is responsible
  for displaying the game and handling ball physics.

* Finally, `pkg/pong/include/logging.h` is a patched version of the pong server
  logging header which allows redirecting its standard output to a physical or
  virtual framebuffer.

All programs under `pkg` also contain examples in the form of appropriate dummy
client programs and demo ned scripts.

## Installation and Startup

You will need (on Linux):
* `gcc` version 8.\*
* `flex` & `bison`
* `dialog`

You should be able to get the pong demo to run simply by following these steps:

1. Obtain the source code of the L4Re version used in this lab

```
$> git clone https://os.inf.tu-dresden.de/repo/git/kpr.git
```

2. Setup and build Fiasco and L4Re for x86. This will only work if `gcc` points
to `gcc` version 8.\* on your system.

```
$> kpr
$> make setup # choose x86
$> make # this will take a while, use -j ... for possible speedup
```

3. Copy the source code in this directory into the L4Re source tree, build it
and run the demo.

```
./install.sh PATH_TO_KPR # located in the same directory as this README
```

where `PATH_TO_KPR` is the absolute path to the git repository obtained in step 1.

This should copy all relevant files into the L4Re source tree, compile them
and automatically run `make qemu` (with the modified `modules.list` found under
`conf`).

Choose the `pong-demo` entry to start a game of pong, the left paddle is
controlled with the `d` and `f` keys and the right one with the `j` and `k`
keys. The `c` key switches to the log framebuffer which logs the remaining
lives for both players. These key bindings can also be modified via command
line arguments in `pong_client_demo.lua`. For details on the other two demos,
consult the corresponding `.lua` files.
