--[[

Starts a framebuffer log server and three demo clients. The clients continually
produce log output which the server tags with a client-unique identifier and
displays by directly writing to the physical framebuffer. The log server
supports continuous scrolling and foreground and background text color can be
customized via command line options.

--]]

local L4 = require 'L4'

local ld = L4.default_loader

local vbus  = ld:new_channel()
local fbdrv = ld:new_channel()
local log   = ld:new_channel()

ld:start(
  {
    caps = {
      fbdrv  = vbus:svr(),
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
      vbus = vbus
    }
  },
  'rom/fb-drv -m 0x117'
)

ld:start(
  {
    caps = {
      fblog = log:svr(),
      fb    = fbdrv
    },
    log = {'log-svr', 'blue'}
  },
  'rom/fb-log --bg-color=blue'
)

ld:start(
  {
    caps = {
      fblog = log:create(0, 'client 1', 'yellow')
    },
    log = {'log-cli1', 'green'}
  },
  'rom/fb-log-demo-client'
)

ld:start(
  {
    caps = {
      fblog = log:create(0, 'client 2', 'magenta')
    },
    log = {'log-cli2', 'green'}
  },
  'rom/fb-log-demo-client'
)

ld:start(
  {
    caps = {
      fblog = log:create(0, 'client 3', 'teal')
    },
    log = {'log-cli3', 'green'}
  },
  'rom/fb-log-demo-client'
)
