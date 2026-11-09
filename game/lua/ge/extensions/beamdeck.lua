-- Telemetry is read-only; the garage accepts explicit, bounded local actions.
local M = {}
local udp, timer, clock = nil, 0, 0
local vehicleData, vehicleId, vehicleSample = {}, nil, -10
local sessionId = tostring(os.time())
local lastError = -60
local metadata, metadataTime = {}, -10
local models, allowedModels = nil, {}
local catalogQueue, extensionLoaded = {}
local completed, completedOrder = {}, {}

local function call(module, method, ...)
  if module and type(module[method]) == 'function' then
    local ok, value = pcall(module[method], ...)
    if ok then return value end
  end
end

local function count(t)
  local n = 0
  if type(t) == 'table' then for _ in pairs(t) do n = n + 1 end end
  return n
end

local function short(s, size)
  return type(s) == 'string' and s:sub(1, size or 160) or nil
end

local function receiveVehicle(id, data)
  if id ~= vehicleId or type(data) ~= 'table' then return end
  vehicleData, vehicleSample = data, clock
end

local vehicleCommand = [[
local e = electrics.values
local engine = powertrain and powertrain.getDevice('mainEngine')
local function active(x) return x == true or (type(x) == 'number' and x > 0) end
local d = {rpm=e.rpm, gear=e.gearIndex, fuel=e.fuel, throttle=e.throttle,
  brake=e.brake, coolant=e.watertemp, oil=e.oiltemp, steering=e.steering_input,
  fuelVolume=e.fuelVolume, fuelCapacity=e.fuelCapacity, boost=e.turboBoost and e.turboBoost / 14.504,
  wheelSpeed=e.wheelspeed, redline=engine and engine.maxRPM,
  lateralG=sensors and sensors.gx2 and sensors.gx2 / 9.81,
  longitudinalG=sensors and sensors.gy2 and sensors.gy2 / 9.81,
  lights=active(e.lights_state), leftSignal=active(e.signal_left_input),
  rightSignal=active(e.signal_right_input), abs=active(e.abs),
  esc=active(e.esc), engineRunning=active(e.running)}
obj:queueGameEngineLua("if extensions.beamdeck then extensions.beamdeck.receiveVehicle(" ..
  tostring(obj:getID()) .. "," .. serialize(d) .. ") end")
]]

local function send(data, targetPort)
  if not udp then return end
  data.schema, data.source, data.sessionId = 1, 'beamdeck', sessionId
  local bytes = jsonEncode(data)
  if #bytes <= 32768 then udp:send(bytes) end
end

local function preview(path)
  if type(path) ~= 'string' or not path:match('^/vehicles/') then return nil end
  local origin = FS and FS.getFileOriginInfo and FS:getFileOriginInfo(path) or {}
  return {path=path, archive=origin.archivePath, real=FS and FS.getFileRealPath and FS:getFileRealPath(path)}
end

local function queueItems(kind, requestId, items, model)
  local truncated = #items > 2048
  local total = math.min(#items, 2048)
  for start = 1, math.max(1, total), 12 do
    local chunk = {}
    for i=start, math.min(start+11,total) do chunk[#chunk+1] = items[i] end
    catalogQueue[#catalogQueue+1] = {kind=kind,requestId=requestId,model=model,items=chunk,
      chunk=math.floor((start-1)/12),complete=start+11>=total,truncated=truncated}
  end
end

local function validConfig(key)
  return type(key) == 'string' and #key > 0 and #key <= 640 and key ~= '.' and key ~= '..'
    and not key:find('[/\\%c]')
end

local function owned(id, multiplayer)
  if not multiplayer then return true end
  return call(MPVehicleGE, 'isOwn', id) == true
end

local function buildMetadata()
  local data = {activeVehicles = {}}
  local mp = MPCoreNetwork or (extensions and extensions.MPCoreNetwork)
  local multiplayer = call(mp, 'isMPSession') == true
  if be and be.getObject then
    for i = 0, math.min(be:getObjectCount(), 128) - 1 do
      local v = be:getObject(i)
      if v then
        local model = short(v:getJBeamFilename(), 80)
        data.activeVehicles[#data.activeVehicles + 1] = {
          id = v:getID(), model = model, name = model, owned = owned(v:getID(), multiplayer)
        }
      end
    end
  end
  return data
end

local function catalog(requestId)
  if not models then
    models, allowedModels = {}, {}
    local result = call(core_vehicles, 'getModelList')
    if not result or type(result.models) ~= 'table' then error('Vehicle catalog unavailable') end
    for key, model in pairs(result.models) do
      local id = type(model) == 'table' and (model.key or key) or key
      if type(model) == 'table' and type(id) == 'string' and id:match('^[%w_-]+$') and #id <= 80 and #models < 2048 then
        allowedModels[id] = true
        models[#models + 1] = {id=id, name=short(model.Name or id),
          brand=short(model.Brand or '', 80), type=short(model.Type or '', 80), preview=preview(model.preview)}
      end
    end
    table.sort(models, function(a,b) return ((a.brand or '') .. (a.name or '')) < ((b.brand or '') .. (b.name or '')) end)
  end
  queueItems('catalog', requestId, models)
end

local function configsForModel(model)
  local out = {}
  if type(model) ~= 'string' or not model:match('^[%w_-]+$') or #model > 80 then error('Invalid model identifier') end
  local data = call(core_vehicles, 'getModel', model)
  if type(data) ~= 'table' or type(data.configs) ~= 'table' then error('Configuration catalog unavailable') end
  if data and type(data.configs) == 'table' then
    for key, config in pairs(data.configs) do
      if validConfig(key) and type(config) == 'table' then
        out[#out + 1] = {key=key, name=short(config.Configuration or config.Name or key),
          description=short(config.Description or '',256), preview=preview(config.preview),
          custom=config.Source == 'Custom', source=short(config.Source or '',80),
          drivetrain=short(config.Drivetrain or '',80), transmission=short(config.Transmission or '',80)}
      end
    end
  end
  table.sort(out, function(a, b) return ((a.slot or '') .. (a.name or '')) < ((b.slot or '') .. (b.name or '')) end)
  return out
end

  -- Cheat-menu state for the current vehicle. Values re-apply every ~2 s so a
  -- physics reset or vehicle reload restores them automatically; the vehicle
  -- Lua keeps the original curve on first touch, so repeats are idempotent.
  local tuning = {}

  local function tuningVehicleCommand(power, damageImmune)
    local factor = string.format('%.6g', power or 1)
    return string.format([[
local factor = %s
local immune = %s
local engines = powertrain and powertrain.getDevicesByCategory and
  powertrain.getDevicesByCategory('engine') or {}
for _, d in pairs(engines) do
  if d.torqueCurve then
    if not d.beamdeckBase then
      d.beamdeckBase, d.beamdeckIdle = {}, d.idleTorque
      for k, val in pairs(d.torqueCurve) do d.beamdeckBase[k] = val end
    end
    for k, val in pairs(d.beamdeckBase) do d.torqueCurve[k] = val * factor end
    if d.beamdeckIdle then d.idleTorque = d.beamdeckIdle * factor end
  end
end
for _, beam in pairs(v.data.beams or {}) do
  if type(beam.beamDeform) == 'number' and type(beam.beamStrength) == 'number' then
    obj:setBeamDeform(beam.cid, immune and 1e30 or beam.beamDeform)
    obj:setBeamStrength(beam.cid, immune and 1e30 or beam.beamStrength)
  end
end
]], factor, damageImmune == true and 'true' or 'false')
  end

  local function applyTuning()
    local command = tuningVehicleCommand(tuning.power, tuning.damage)
    local vehicle = getPlayerVehicle and getPlayerVehicle(0)
    if vehicle then vehicle:queueLuaCommand(command) end
    tuning.appliedTo = vehicle and vehicle:getID()
    tuning.appliedTime = clock
  end

  local function tuningAction(command, result)
    local vehicle = getPlayerVehicle and getPlayerVehicle(0)
    if not vehicle then result(false,'Select a vehicle first.'); return end
    if not owned(vehicle:getID(), call(MPCoreNetwork, 'isMPSession') == true) then
      result(false,'Select your own vehicle first.'); return
    end
    local power, gravity, immune = command.enginePower, command.gravity, command.damageImmunity
    if power ~= nil and (type(power) ~= 'number' or power ~= power or power < 0.5 or power > 3) then
      result(false,'Engine power multiplier must be between 0.5 and 3.'); return
    end
    if gravity ~= nil and (type(gravity) ~= 'number' or gravity ~= gravity or gravity < -30 or gravity > 0) then
      result(false,'Gravity must be between -30 and 0 m/s2.'); return
    end
    if immune ~= nil and type(immune) ~= 'boolean' then
      result(false,'Damage immunity must be on or off.'); return
    end
    if gravity ~= nil and (not core_environment or type(core_environment.setGravity) ~= 'function') then
      result(false,'Gravity controls unavailable.'); return
    end
    if power ~= nil then tuning.power = power end
    if immune ~= nil then tuning.damage = immune end
    if gravity ~= nil and type(core_environment) == 'table'
      and type(core_environment.setGravity) == 'function' then
      if not pcall(core_environment.setGravity, gravity) then
        result(false,'The game rejected the gravity change.'); return
      end
      tuning.gravity = gravity
    end
    applyTuning()
    result(true,'Tuning sent to the selected vehicle. Gravity affects the whole world.')
  end

  local function funAction(name, result)
    local vehicle = getPlayerVehicle and getPlayerVehicle(0)
    if (name == 'repair' or name == 'ignite' or name == 'extinguish' or name == 'deflate') then
      if not vehicle then result(false,'No vehicle selected.'); return end
      if not owned(vehicle:getID(), call(MPCoreNetwork, 'isMPSession') == true) then
        result(false,'Select your own vehicle first.'); return
      end
      if name == 'repair' and vehicle:getVelocity():length() > 1 then
        result(false,'Stop the vehicle before repairing it.'); return
      end
      local ok, err = true, nil
      if name == 'repair' then
        ok = type(spawn) == 'table' and type(spawn.safeTeleport) == 'function' and
          pcall(spawn.safeTeleport, vehicle, vehicle:getPosition(),
            quatFromDir(vehicle:getDirectionVector()), nil, nil, nil, nil, true)
      elseif name == 'ignite' then
        ok = pcall(function() vehicle:queueLuaCommand('fire.igniteVehicle()') end)
      elseif name == 'extinguish' then
        ok = pcall(function() vehicle:queueLuaCommand('fire.extinguishVehicle()') end)
      else
        ok = pcall(function() vehicle:queueLuaCommand('beamstate.deflateTires()') end)
      end
      result(ok, ok and ({repair='Vehicle restored.', ignite='Fire lit.',
        extinguish='Fire out.', deflate='Tires deflated.'})[name] or
        ('Game refused the request: ' .. tostring(err or 'unknown failure'):sub(1,140)))
      return true
    end
    return false
  end

  local function action(command)
  if type(command) ~= 'table' or command.schema ~= 1 or command.source ~= 'beamdeck-control'
    or command.sessionId ~= sessionId or type(command.requestId) ~= 'string'
    or #command.requestId == 0 or #command.requestId > 80 then return end
  local name, id = command.action, command.requestId
  if completed[id] then send(completed[id]); return end
  local function result(ok, message)
    local response = {kind='result',requestId=id,ok=ok,message=message}
    if name == 'spawn' or name == 'replace' or name == 'switch' or name == 'enableBeamMP'
      or name == 'tuning' or name == 'repair' or name == 'ignite' or name == 'extinguish' or name == 'deflate' then
      completed[id] = response; completedOrder[#completedOrder+1] = id
      if #completedOrder > 128 then completed[table.remove(completedOrder,1)] = nil end
    end
    send(response)
  end
  if name == 'catalog' then
    local ok, err = pcall(catalog,id)
    if not ok then result(false,'Could not read vehicle catalog: '..tostring(err):sub(1,160)) end
    return
  end
  if name == 'configs' then
    local ok, data = pcall(configsForModel, command.model)
    if ok then queueItems('configs',id,data,command.model) else result(false, 'Could not list configurations: ' .. tostring(data):sub(1, 160)) end
    return
  end
  if name == 'enableBeamMP' then
    if not core_modmanager or type(core_modmanager.activateMod) ~= 'function' then result(false,'Mod manager unavailable'); return end
    local mods = call(core_modmanager,'getMods')
    if type(mods) ~= 'table' or not mods.multiplayerbeammp then result(false,'Install the BeamMP game mod first.'); return end
    local ok, err = pcall(core_modmanager.activateMod,'multiplayerbeammp')
    local active = ok and mods.multiplayerbeammp.active == true
    result(active, active and 'BeamMP mod enabled.' or ('Could not enable BeamMP: '..tostring(err or 'mod remained inactive'):sub(1,160)))
    return
  end
  if name ~= 'spawn' and name ~= 'replace' and name ~= 'switch' and name ~= 'tuning'
    and not ({repair=true, ignite=true, extinguish=true, deflate=true})[name] then
    result(false,'Unsupported action'); return
  end
  if name == 'tuning' then
    tuningAction(command, result)
    return
  end
  if ({repair=true, ignite=true, extinguish=true, deflate=true})[name] then
    funAction(name, result)
    return
  end
  local current = getPlayerVehicle and getPlayerVehicle(0)
  if current and current:getVelocity():length() > 1 then result(false,'Stop the vehicle before using garage actions.'); return end
  if not getMissionFilename or (getMissionFilename() or '') == '' then result(false,'Load a map before using the garage.'); return end
  local multiplayer = call(MPCoreNetwork, 'isMPSession') == true
  if name == 'switch' then
    local targetId = command.vehicleId
    if type(targetId) ~= 'number' or targetId < 0 or targetId % 1 ~= 0 or not owned(targetId,multiplayer) then
      result(false,'Only your own vehicles can be selected.'); return
    end
    local vehicle = be:getObjectByID(targetId)
    if not vehicle then result(false,'Vehicle no longer exists.'); return end
    be:enterVehicle(0,vehicle)
    result(true,'Vehicle selected.'); return
  end
  local model, key = command.model, command.config
  if type(model) ~= 'string' or not model:match('^[%w_-]+$') or #model > 80 or not validConfig(key) then
    result(false,'Choose a vehicle variant from the loaded game catalog.'); return
  end
  local data = call(core_vehicles, 'getModel', model)
  local selected = data and data.configs and data.configs[key]
  if type(selected) ~= 'table' or type(selected.pcFilename) ~= 'string' then
    result(false,'Choose a vehicle variant from the loaded game catalog.'); return
  end
  -- Use the same public functions as the in-game vehicle selector. BeamMP's
  -- overrides and normal server restrictions remain in effect.
  local method = name == 'spawn' and 'spawnNewVehicle' or 'replaceVehicle'
  if not core_vehicles or type(core_vehicles[method]) ~= 'function' then result(false,'Vehicle API unavailable.'); return end
  if name == 'replace' and current and not owned(current:getID(), multiplayer) then
    result(false,'Cannot replace another player\'s vehicle.'); return
  end
  local ok, err = pcall(core_vehicles[method], model, {config=selected.pcFilename})
  result(ok, ok and 'Vehicle request submitted. The game or server may still refuse it.' or ('Game refused the request: '..tostring(err):sub(1,160)))
end

local function snapshot()
  local mp = MPCoreNetwork or (extensions and extensions.MPCoreNetwork)
  local vehicles = MPVehicleGE or (extensions and extensions.MPVehicleGE)
  local multiplayer = call(mp, 'isMPSession') == true
  local data = {
    schema = 1, source = 'beamdeck', sessionId = sessionId,
    version = short(beamng_versionb), beammpLoaded = mp ~= nil,
    bridgeVersion = '0.4.0', controlsAvailable = true, sampleHz = 20,
    gameFolder=FS and FS.getGamePath and FS:getGamePath(),
    userFolder=FS and FS.getUserPath and FS:getUserPath(),
    multiplayer = multiplayer, joining = call(mp, 'isGoingMPSession') == true,
    map = short(getMissionFilename and getMissionFilename()),
    paused = call(simTimeAuthority, 'getPause') == true,
    fps = VariableRegistry and tonumber(VariableRegistry.get('$fps::avg')) or nil,
    cpuMs = VariableRegistry and tonumber(VariableRegistry.get('$fps::cpuTime')) or nil,
    gpuMs = VariableRegistry and tonumber(VariableRegistry.get('$fps::gpuTime')) or nil,
    vehicleCount = be and be:getObjectCount() or nil,
    telemetry = {}
  }
  if clock - metadataTime > .5 then metadata, metadataTime = buildMetadata(), clock end
  data.activeVehicles = metadata.activeVehicles
  if data.map then data.map = data.map:match('/levels/([^/]+)') or data.map end
  local vehicle = getPlayerVehicle and getPlayerVehicle(0)
  local id = vehicle and vehicle:getID()
  if id ~= vehicleId then vehicleData, vehicleSample, vehicleId = {}, -10, id end
  if vehicle and tuning.appliedTo == id and (tuning.power or tuning.damage) then
    -- Re-apply active cheat settings so resets and reloads keep them; repeated
    -- application is safe because each engine keeps its untouched base curve.
    if tuning.appliedTo ~= id or clock - (tuning.appliedTime or -10) > 2 then applyTuning() end
  end
  if vehicle then
    data.vehicle = short(vehicle:getJBeamFilename())
    data.vehicleId = id
    data.vehicleName = data.vehicle
    if clock - vehicleSample < 1.5 then
      for k, v in pairs(vehicleData) do data.telemetry[k] = v end
    end
    -- Speed uses world velocity, not driven wheel speed, so wheelspin is honest.
    data.telemetry.speed = vehicle:getVelocity():length()
    vehicle:queueLuaCommand(vehicleCommand)
  end
  if multiplayer then
    local server = call(mp, 'getCurrentServer')
    data.server = type(server) == 'table' and short(server.name) or nil
    data.players = {}
    local players = call(vehicles, 'getPlayers')
    data.playerCount = type(players) == 'table' and count(players) or nil
    local mpVehicles = call(vehicles, 'getVehicles')
    data.mpVehicleCount = type(mpVehicles) == 'table' and count(mpVehicles) or nil
    if type(players) == 'table' then
      for _, p in pairs(players) do
        if type(p) == 'table' and #data.players < 128 then
          local ping = tonumber(p.ping)
          if ping and ping < 0 then ping = nil end
          data.players[#data.players + 1] = {
            name = short(p.name, 64), ['local'] = p.isLocal == true,
            ping = ping, vehicles = p.vehicles and count(p.vehicles.IDs) or nil
          }
          if p.isLocal then data.ping = ping end
        end
      end
    end
    table.sort(data.players, function(a, b)
      if a['local'] ~= b['local'] then return a['local'] end
      return (a.name or '') < (b.name or '')
    end)
  end
  return data
end

local function onExtensionLoaded()
  setExtensionUnloadMode(M, 'manual')
  local ok, socket = pcall(require, 'socket')
  if not ok then log('E', 'beamdeck', 'LuaSocket unavailable: ' .. tostring(socket)); return end
  udp = socket.udp()
  if not udp then log('E', 'beamdeck', 'Cannot create telemetry socket'); return end
  udp:settimeout(0)
  udp:setpeername('127.0.0.1',44444)
  log('I', 'abm', 'ABM 0.4: telemetry, garage actions, and tuning on localhost:44444')
end

local function onUpdate(dtReal)
  clock = clock + (dtReal or 0)
  timer = timer + (dtReal or 0)
  if not udp then return end
  -- Connected UDP only accepts datagrams from the local receiver's port.
  if udp.receive then
    for _=1,8 do
      local bytes = udp:receive()
      if not bytes then break end
      if #bytes <= 4096 then
        local ok, data = pcall(jsonDecode,bytes)
        if ok then
          local worked, err = pcall(action,data)
          if not worked and type(data) == 'table' then send({kind='result',requestId=data.requestId,ok=false,message='Game action failed: '..tostring(err):sub(1,160)}) end
        end
      end
    end
  end
  -- Spread large model catalogs across frames so UDP buffers and frame time
  -- are not overwhelmed by a heavily modded installation.
  for _=1,4 do
    local packet = table.remove(catalogQueue,1)
    if not packet then break end
    send(packet)
  end
  if timer < 0.05 then return end
  timer = 0
  local ok, err = pcall(function()
    send(snapshot())
  end)
  if not ok and clock - lastError > 60 then
    lastError = clock
    log('E', 'beamdeck', 'Telemetry read failed: ' .. tostring(err))
  end
end

M.receiveVehicle = receiveVehicle
M.onExtensionLoaded = onExtensionLoaded
M.onUpdate = onUpdate
M.onClientStartMission = function()
  sessionId = tostring(os.time()) .. '-' .. tostring(clock)
  vehicleData, vehicleId, vehicleSample = {}, nil, -10
  metadataTime, models, allowedModels = -10, nil, {}
  catalogQueue, extensionLoaded = {}, nil
  completed, completedOrder = {}, {}
  tuning = {}
end
M.onExtensionUnloaded = function() if udp then udp:close(); udp = nil end end
return M
