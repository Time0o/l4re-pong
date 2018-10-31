TU Dresden _Complex Lab Microkernel-Based Operating Systems_ solution.

# Installation

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
