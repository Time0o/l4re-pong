--[[

Keyboard driver demo, starts two clients listening to different keys via a
session interface, press 'A' or 'B' to trigger a response from the first and
'A' or 'C' to trigger one from the second client.

--]]

local L4 = require 'L4'

local ld = L4.default_loader

local vbus_gui = ld:new_channel()
local keyboard = ld:new_channel()

ld:start(
  {
    caps = {
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
      keyboard = keyboard:svr(),
      icu      = L4.Env.icu,
      vbus     = vbus_gui
    },
    log = {'kyb-drv', 'blue'}
  },
  'rom/keyboard-drv'
)

ld:start(
  {
    caps = {
      keyboard = keyboard:create(0, 'A', 'B'),
    },
    log = {'client1', 'green'}
  },
  'rom/keyboard-drv-demo-client A B'
)

ld:start(
  {
    caps = {
      keyboard = keyboard:create(0, 'A', 'C'),
    },
    log = {'client2', 'red'}
  },
  'rom/keyboard-drv-demo-client A C'
)
