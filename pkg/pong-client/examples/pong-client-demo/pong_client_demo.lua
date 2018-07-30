--[[

Starts a game of pong, i.e. a pong server and a pong client. The pong client
can be controlled via the keyboard server which it periodically queries for
held keys. Per default the first paddle is controlled by the 'd' and 'f' and
the second by the 'h' and 'j' keys, this can be changed via command line options
as shown below.

Several servers also produce graphical log output via a framebuffer log server.
The framebuffer log server's graphical output is multiplexed with the pong
servers graphical output via a framebuffer multiplexer server which is
connected to another client also controlled via keyboard. Per default the
'ENTER' key switches between the pong game and the graphical console, this can
also be customized.

--]]

local L4 = require 'L4'

local ld = L4.default_loader

local vbus_gui         = ld:new_channel()
local vbus_fbdrv       = ld:new_channel()
local fbdrv            = ld:new_channel()
local fbmux            = ld:new_channel()
local fblog            = ld:new_channel() -- XXX
local virtfb_main      = ld:new_channel()
local virtfb_secondary = ld:new_channel()
local keyboard         = ld:new_channel()
local pong             = ld:new_channel()

ld:start(
  {
    caps = {
      fbdrv  = vbus_fbdrv:svr(),
      gui    = vbus_gui:svr(),
      icu    = L4.Env.icu,
      sigma0 = L4.cast(L4.Proto.Factory, L4.Env.sigma0):create(L4.Proto.Sigma0)
    }
  },
  'rom/io rom/x86-legacy.devs rom/x86-fb.io'
)

ld:start(
  {
    caps = {
      fb   = fbdrv:svr(),
      vbus = vbus_fbdrv
    }
  },
  'rom/fb-drv -m 0x117'
)

ld:start(
  {
    caps = {
      virtfb_main      = virtfb_main:svr(),
      virtfb_secondary = virtfb_secondary:svr(),
      fbmux            = fbmux:svr(),
      fb               = fbdrv
    },
    log = {'mux', 'red'}
  },
  'rom/fb-mux'
)

ld:start(
  {
    caps = {
      PongServer = pong:svr(),
      vesa       = virtfb_main
    },
    log = {'pong-svr', 'blue'}
  },
  'rom/pong-server'
)

ld:start(
  {
    caps = {
      fblog = fblog:svr(),
      fb    = virtfb_secondary
    },
    log = {'log-svr', 'green'}
  },
  'rom/fb-log --bg-color=blue'
)

ld:start(
  {
    caps = {
      pongserver = pong,
      keyboard   = keyboard,
      fbmux      = fbmux
    },
    log = {'pong-cli', 'magenta'}
  },
  'rom/pong-client --paddle-left-up=s --paddle-left-down=a '..
  '--paddle-right-up=m --paddle-right-down=n --switch-console=c'
)

ld:start(
  {
    caps = {
      keyboard = keyboard:svr(),
      icu      = L4.Env.icu,
      vbus     = vbus_gui
    },
    log = {'kyb-drv', 'yellow'}
  },
  'rom/keyboard-drv'
)
