-- ABM vehicle tuning: fixed actions only, with preflight before mutation.
local M = {}
local gearBase
local gearFields = {'gearRatios', 'gearWhineCoefsOutput', 'gearWhineCoefsInput', 'synchroWear',
  'previouslyReportedSynchroWear', 'shiftAllowedNonClutchAVDifference', 'shiftRequiredClutchInput',
  'shiftMaxSynchroAVCapability', 'shiftMaxSynchroRate', 'synchroWearCoef'}

local function finite(n) return type(n) == 'number' and n == n and math.abs(n) < math.huge end
local function copy(t)
  local out = {}
  for k, value in pairs(t or {}) do out[k] = value end
  return out
end
local function between(n, low, high) return finite(n) and n >= low and n <= high end

local function tuning(settings)
  local factor, immune = settings.enginePower, settings.damageImmunity
  if not finite(factor) or factor <= 0 or type(immune) ~= 'boolean' then error('Enter a finite positive power multiplier.') end
  local engines = powertrain and powertrain.getDevicesByCategory and powertrain.getDevicesByCategory('engine') or {}
  -- Validate every derived value before changing anything. There is no fixed
  -- power ceiling, but an overflowing torque curve cannot enter the physics VM.
  for _, device in pairs(engines) do
    for _, value in pairs(device.beamdeckBase or device.torqueCurve or {}) do
      if not finite(value * factor) then error('Power would overflow this engine torque curve.') end
    end
    local idle = device.beamdeckIdle or device.idleTorque
    if idle and not finite(idle * factor) then error('Power would overflow idle torque.') end
  end
  for _, device in pairs(engines) do
    if device.torqueCurve then
      device.beamdeckBase = device.beamdeckBase or copy(device.torqueCurve)
      device.beamdeckIdle = device.beamdeckIdle or device.idleTorque
      for rpm, value in pairs(device.beamdeckBase) do device.torqueCurve[rpm] = value * factor end
      if device.beamdeckIdle then device.idleTorque = device.beamdeckIdle * factor end
    end
  end
  for _, beam in pairs(v.data.beams or {}) do
    if finite(beam.beamDeform) and finite(beam.beamStrength) then
      obj:setBeamDeform(beam.cid, immune and 1e30 or beam.beamDeform)
      obj:setBeamStrength(beam.cid, immune and 1e30 or beam.beamStrength)
    end
  end
  return 'Power and beam protection applied.'
end

local function apply(settings)
  local grip, sliding, brake, steering = settings.tireGrip, settings.slidingGrip, settings.brakePower, settings.steeringResponse
  if not between(grip, 0, 10) or not between(sliding, 0, 10) or not between(brake, 0, 10)
    or not between(steering, 0.1, 3) then error('Invalid handling values.') end
  local tireNodes, steeringHydros = {}, {}
  for wheelId, wheel in pairs(v.data.wheels or {}) do
    local live = wheels and wheels.wheels and wheels.wheels[wheelId]
    if not live or (not live.isTireDeflated and not live.isBroken) then
      for _, cid in pairs(wheel.treadNodes or {}) do
        if v.data.nodes[cid] then tireNodes[cid] = v.data.nodes[cid] end
      end
    end
  end
  for _, hydro in pairs(hydros and hydros.hydros or {}) do
    if hydro.inputSource == 'steering_input' or hydro.inputSource == 'steering' then
      if type(hydro.inputFactor) == 'number' then steeringHydros[#steeringHydros+1] = hydro end
    end
  end
  if (grip ~= 1 or sliding ~= 1) and not next(tireNodes) then error('This vehicle has no supported tire tread nodes.') end
  if next(tireNodes) and type(obj.setNodeFrictionSlidingCoefs) ~= 'function' then error('Tire friction API unavailable.') end
  if brake ~= 1 and (not wheels or type(wheels.scaleBrakeTorque) ~= 'function') then error('Brake API unavailable.') end
  if steering ~= 1 and #steeringHydros == 0 then error('This vehicle has no supported steering hydros.') end
  for cid, node in pairs(tireNodes) do
    local static = type(node.frictionCoef) == 'number' and node.frictionCoef or 1
    local slide = type(node.slidingFrictionCoef) == 'number' and node.slidingFrictionCoef or static
    obj:setNodeFrictionSlidingCoefs(cid, static * grip, slide * sliding)
  end
  if wheels and wheels.scaleBrakeTorque then wheels.scaleBrakeTorque(brake) end
  for _, hydro in ipairs(steeringHydros) do
    hydro.abmInputFactor = hydro.abmInputFactor or hydro.inputFactor
    hydro.inputFactor = hydro.abmInputFactor * steering
  end
  return 'Handling applied. Tire damage and other controllers may override it.'
end

-- Runtime expansion is deliberately restricted to the verified manual gearbox.
-- Reverse/neutral are preserved. All indexed synchro/sound tables must grow
-- together; changing maxGearIndex alone causes nil-index physics failures.
local function gears(ratios)
  if type(ratios) ~= 'table' or #ratios < 1 then error('Enter forward gear ratios.') end
  for _, ratio in ipairs(ratios) do
    if not between(ratio, 0.01, 100) then error('Ratios must be finite and between 0.01 and 100.') end
  end
  local devices = powertrain and powertrain.getDevicesByType and powertrain.getDevicesByType('manualGearbox') or {}
  local gearbox, count
  count = 0
  for _, device in pairs(devices) do gearbox, count = device, count + 1 end
  if count ~= 1 or type(gearbox.setGearIndex) ~= 'function' then error('Requires exactly one supported manual gearbox; automatic/CVT/DCT are not supported.') end
  if type(powertrain.calculateTreeInertia) ~= 'function' then error('Powertrain inertia API unavailable.') end
  local fields = gearFields
  local template = gearbox.maxGearIndex
  for _, field in ipairs(fields) do
    if type(gearbox[field]) ~= 'table' or gearbox[field][template] == nil then error('Unsupported gearbox table: '..field) end
  end
  if not gearBase or gearBase.device ~= gearbox then
    gearBase = {device=gearbox, max=template, count=gearbox.gearCount,
      minRatio=gearbox.minGearRatio,maxRatio=gearbox.maxGearRatio}
    for _, field in ipairs(fields) do gearBase[field] = copy(gearbox[field]) end
  end
  -- The GE bridge requires a stopped vehicle; select neutral before resizing.
  gearbox:setGearIndex(0)
  for _, field in ipairs(fields) do
    local base = gearBase[field]
    local out = copy(base)
    for index in pairs(out) do if type(index)=='number' and index > 0 then out[index]=nil end end
    for index=1,#ratios do
      out[index] = field == 'gearRatios' and ratios[index] or base[math.min(index,gearBase.max)]
    end
    gearbox[field] = out
  end
  gearbox.maxGearIndex = #ratios
  gearbox.gearCount = #ratios + math.abs(gearbox.minGearIndex or -1)
  local low, high = math.huge, 0
  for _, ratio in pairs(gearbox.gearRatios) do
    if ratio ~= 0 then low, high = math.min(low, math.abs(ratio)), math.max(high, math.abs(ratio)) end
  end
  gearbox.minGearRatio, gearbox.maxGearRatio = low, high
  powertrain.calculateTreeInertia()
  return 'Manual gearbox ratios applied. Shift with sequential up/down controls; reset the vehicle to restore stock.'
end

local function stability()
  local esc = controller and controller.getController and controller.getController('esc')
  if not esc or not esc.isExisting or type(esc.toggleESCMode) ~= 'function' then error('This vehicle has no supported ESC controller.') end
  esc.toggleESCMode()
  return 'Cycled the vehicle ESC/traction-control mode. Check its in-game indicator.'
end

local function execute(action, settings)
  if action == 'tuning' then return tuning(settings)
  elseif action == 'handling' then return apply(settings)
  elseif action == 'gears' then return gears(settings.ratios)
  elseif action == 'stability' then return stability()
  end
  error('Unsupported vehicle tuning action.')
end

M.execute = execute
M.onReset = function()
  -- Vehicle extensions reset before powertrain/controllers. Restore dimensions
  -- first so their stock-ratio reset never indexes an expanded gearbox.
  if gearBase then
    local device = gearBase.device
    for _, field in ipairs(gearFields) do device[field] = copy(gearBase[field]) end
    device.maxGearIndex, device.gearCount = gearBase.max, gearBase.count
    device.minGearRatio, device.maxGearRatio = gearBase.minRatio, gearBase.maxRatio
    gearBase = nil
  end
  for _, hydro in pairs(hydros and hydros.hydros or {}) do
    if hydro.abmInputFactor then hydro.inputFactor, hydro.abmInputFactor = hydro.abmInputFactor, nil end
  end
end
return M
