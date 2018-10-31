--[[

Starts two framebuffer log servers, each having one client producing continuous
log output. Both of these servers in turn write to separate virtual framebuffers
provided by a framebuffer mux server. A framebuffer mux demo client then
periodically instructs the framebuffer mux server to alternatingly map the
framebuffer log servers' virtual framebuffers to the physical framebuffer.

--]]

local L4 = require 'L4'

local ld = L4.default_loader

local vbus_fbdrv       = ld:new_channel()
local vbus_gui         = ld:new_channel()
local fbdrv            = ld:new_channel()
local fbmux            = ld:new_channel()
local virtfb_main      = ld:new_channel()
local virtfb_secondary = ld:new_channel()
local fblog_main       = ld:new_channel()
local fblog_secondary  = ld:new_channel()

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
      fblog = fblog_main:svr(),
      fb    = virtfb_main
    },
    log = {'log-svr1', 'green'}
  },
  'rom/fb-log'
)

ld:start(
  {
    caps = {
      fblog = fblog_secondary:svr(),
      fb    = virtfb_secondary
    },
    log = {'log-svr2', 'green'}
  },
  'rom/fb-log'
)

ld:start(
  {
    caps = {
      fblog = fblog_main:create(0, 'cli1')
    },
    log = {'log-cli1', 'yellow'}
  },
  'rom/fb-log-demo-client'
)

ld:start(
  {
    caps = {
      fblog = fblog_secondary:create(0, 'cli2')
    },
    log = {'log-cli2', 'yellow'}
  },
  'rom/fb-log-demo-client'
)

ld:start(
  {
    caps = {
      fbmux = fbmux,
    },
    log = {'mux-cli', 'magenta'}
  },
  'rom/fb-mux-demo-client'
)
