-- ABM vehicle tuning: fixed actions only, with preflight before mutation.
local M = {version='0.10.0'}
local instance = tostring(obj:getID())..':'..tostring({})..':'..tostring(os.time())
local generation = 0
local revisions = {}
local acceptedPower = nil
local gearBase
local dimensionBase
local engineBases = {}
local rpmBases, clutchBases = {}, {}
local recipe, restorePending, restoreFrames, restoreAge = {}, false, 0, 0
local restoreStatus, restoreError = 'idle', ''
local authorizedUntil, resetPersistence = 0, true
local strengthFields = {'maxTorqueRating', 'maxOverTorqueDamage', 'maxOverRevDamage'}
local protectionMethods = {'lockUp', 'onBreak', 'applyDeformGroupDamage', 'checkHydroLocking', 'scaleOutputTorque'}
local function protected() end
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

local rpmFields = {'maxRPM','maxAV','revLimiterRPM','revLimiterAV','revLimiterMaxAV','invRevLimiterRange'}
local function rpmDevices()
  local devices = powertrain and powertrain.getDevicesByType and powertrain.getDevicesByType('combustionEngine') or {}
  if not next(devices) then error('A supported combustion engine is required.') end
  for _, d in pairs(devices) do
    if d.hasRevLimiter ~= true or not finite(d.idleRPM) or not finite(d.revLimiterRPM)
      or not finite(d.revLimiterAV) or not finite(d.maxRPM) or not finite(d.maxAV)
      or type(d.torqueCurve) ~= 'table' or not ({rpmDrop=true,timeBased=true,soft=true})[d.revLimiterType] then
      error('Unsupported permanent rev limiter. Electric/custom motors are not supported.')
    end
    if d.revLimiterType == 'soft' and not between(d.revLimiterMaxAVOvershoot,.001,100000) then error('Unsupported soft limiter range.') end
  end
  return devices
end
local function rpmCeiling(d)
  local ceiling = (rpmBases[d] and rpmBases[d].revLimiterRPM) or d.revLimiterRPM
  for rpm, torque in pairs(d.beamdeckBase or d.torqueCurve) do
    if finite(rpm) and finite(torque) and torque > 0 then ceiling=math.max(ceiling,rpm) end
  end
  return math.min(30000,ceiling)
end
local function setRPM(value)
  if not between(value,100,30000) or value%1~=0 then error('RPM must be an integer between 100 and 30000.') end
  local devices=rpmDevices()
  for _,d in pairs(devices) do
    if value < d.idleRPM+100 or value > rpmCeiling(d) then error('RPM must exceed idle by 100 and stay within the reported existing-curve ceiling. No synthetic torque is added.') end
  end
  for _,d in pairs(devices) do
    if not rpmBases[d] then
      local base={}; for _,field in ipairs(rpmFields) do base[field]=d[field] end; rpmBases[d]=base
    end
    d.maxRPM,d.revLimiterRPM=value,value
    d.maxAV,d.revLimiterAV=value*math.pi/30,value*math.pi/30
    if d.revLimiterType=='soft' then
      d.revLimiterMaxAV=d.maxAV+d.revLimiterMaxAVOvershoot
      d.invRevLimiterRange=1/d.revLimiterMaxAVOvershoot
    end
  end
  return 'Permanent RPM limiter applied. Torque curves, shift points, ESC/launch temporary limits and physical damage thresholds are unchanged.'
end
local function clutches()
  local devices=powertrain and powertrain.getDevicesByType and powertrain.getDevicesByType('frictionClutch') or {}
  if not next(devices) then error('Requires a supported friction clutch; DCT/automatic/custom clutches are not supported.') end
  for _,d in pairs(devices) do
    if type(d.updateGFX)~='function' or type(d.applyDeformGroupDamage)~='function'
      or not finite(d.thermalEfficiency) or not finite(d.clutchTemperature)
      or not finite(d.damageLockTorqueCoef) or not finite(d.wearLockTorqueCoef) then error('Unsupported clutch damage API.') end
  end
  return devices
end
local function protectClutch(enabled)
  if type(enabled)~='boolean' then error('Clutch protection must be on or off.') end
  local devices=clutches()
  for _,d in pairs(devices) do
    if enabled and (d.clutchPermanentlyDamaged or d.thermalEfficiency < .999 or d.damageLockTorqueCoef < .999) then error('Clutch already damaged or overheated. Repair/reset before enabling protection.') end
    local base=clutchBases[d]
    if base and (d.updateGFX~=base.wrapper or d.applyDeformGroupDamage~=protected) then
      error('Another mod replaced clutch hooks; reset the vehicle before restoring.')
    end
  end
  for _,d in pairs(devices) do
    local base=clutchBases[d]
    if enabled and not base then
      base={updateGFX=d.updateGFX,damage=d.applyDeformGroupDamage,wear=d.wearLockTorqueCoef}
      base.wrapper=function(device)
        device.frictionLossPerUpdate=0
        device.wearLockTorqueCoef=base.wear
      end
      clutchBases[d]=base
      d.updateGFX,d.applyDeformGroupDamage=base.wrapper,protected
    elseif not enabled and base then
      d.updateGFX,d.applyDeformGroupDamage=base.updateGFX,base.damage
      clutchBases[d]=nil
    end
  end
  return enabled and 'Clutch heat/deformation protection ON. Heat simulation is frozen; stock torque capacity and physical breakage remain.'
    or 'Clutch protection OFF; normal heat and deformation hooks restored.'
end

local function restoreEngine(device, base)
  for _, field in ipairs(strengthFields) do device[field] = base.fields[field] end
  device.maxPhysicalAV, device.canFlood = base.maxPhysicalAV, base.canFlood
  for _, method in ipairs(protectionMethods) do device[method] = base.methods[method] end
  if base.thermals then base.thermals.updateGFX = base.thermalUpdate end
end

local function engineStrength(settings)
  local factor, immune = settings.engineStrength, settings.engineIndestructible
  if not between(factor, 0.1, 100) or type(immune) ~= 'boolean' then error('Engine strength must be 0.1–100x, with protection on or off.') end
  local devices = powertrain and powertrain.getDevicesByType and powertrain.getDevicesByType('combustionEngine') or {}
  if not next(devices) then error('Requires a supported combustion engine; electric/custom motors are not supported.') end
  -- Reject partial support before touching any engine in a multi-engine vehicle.
  for _, device in pairs(devices) do
    for _, field in ipairs(strengthFields) do
      if not finite(device[field]) then error('Unsupported engine durability field: '..field) end
    end
    if not finite(device.maxPhysicalAV) or not device.thermals or type(device.thermals.updateGFX) ~= 'function' then error('Unsupported engine thermal/RPM API.') end
    for _, method in ipairs(protectionMethods) do
      if type(device[method]) ~= 'function' then error('Unsupported engine damage hook: '..method) end
    end
    local base = engineBases[device]
    for _, field in ipairs(strengthFields) do
      if not finite((base and base.fields[field] or device[field]) * factor) then error('Engine strength would overflow.') end
    end
  end
  for _, device in pairs(devices) do
    local base = engineBases[device]
    if not base then
      base = {fields={}, methods={}, maxPhysicalAV=device.maxPhysicalAV, canFlood=device.canFlood,
        thermals=device.thermals, thermalUpdate=device.thermals.updateGFX}
      for _, field in ipairs(strengthFields) do base.fields[field] = device[field] end
      for _, method in ipairs(protectionMethods) do base.methods[method] = device[method] end
      engineBases[device] = base
    end
    restoreEngine(device, base)
    for _, field in ipairs(strengthFields) do
      -- A negative torque rating means the stock engine has no torque limit.
      device[field] = base.fields[field] > 0 and base.fields[field] * factor or base.fields[field]
    end
    if immune then
      device.maxTorqueRating, device.maxPhysicalAV, device.canFlood = -1, 1e30, false
      for _, method in ipairs(protectionMethods) do device[method] = protected end
      base.thermals.updateGFX = protected
    end
  end
  return immune and 'Engine protection ON. Thermal simulation is frozen; existing damage is not repaired.'
    or 'Engine strength applied; normal damage hooks restored. Existing damage is not repaired.'
end

local function setPower(factor)
  if not finite(factor) or factor <= 0 then error('Enter a finite positive power multiplier.') end
  local engines = powertrain and powertrain.getDevicesByCategory and powertrain.getDevicesByCategory('engine') or {}
  local supported = false
  for _, device in pairs(engines) do
    if type(device.torqueCurve) == 'table' and next(device.torqueCurve) then supported = true end
    for _, value in pairs(device.beamdeckBase or device.torqueCurve or {}) do
      if not finite(value * factor) then error('Power would overflow this engine torque curve.') end
    end
    local idle = device.beamdeckIdle or device.idleTorque
    if idle and not finite(idle * factor) then error('Power would overflow idle torque.') end
  end
  if not supported then error('This vehicle has no supported engine torque curve.') end
  for _, device in pairs(engines) do
    if device.torqueCurve then
      device.beamdeckBase = device.beamdeckBase or copy(device.torqueCurve)
      device.beamdeckIdle = device.beamdeckIdle or device.idleTorque
      for rpm, value in pairs(device.beamdeckBase) do device.torqueCurve[rpm] = value * factor end
      if device.beamdeckIdle then device.idleTorque = device.beamdeckIdle * factor end
    end
  end
  acceptedPower = factor
  return 'Engine power applied. Beam limits and gravity were not changed.'
end

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
  acceptedPower = factor
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
local function upvalue(fn, wanted)
  if type(fn) ~= 'function' or not debug or type(debug.getupvalue) ~= 'function' then return end
  for index=1,100 do
    local name, value = debug.getupvalue(fn,index)
    if not name then return end
    if name == wanted then return value end
  end
end

local function controllerShiftPoints(gearbox)
  -- BeamNG exposes this getter only to its shift-logic module, not through
  -- mainController. Read named closures, never replace code or guess indices.
  -- Fail closed on changed/modded controllers before touching the powertrain.
  local main = controller and controller.mainController
  local logic = main and upvalue(main.updateGFX,'controlLogicModule')
  local shared = logic and upvalue(logic.init,'sharedFunctions')
  if not shared or type(shared.getShiftPoints) ~= 'function'
    or upvalue(logic.init,'gearbox') ~= gearbox then error('Unsupported shift controller; gearbox left unchanged.') end
  local points = shared.getShiftPoints()
  if type(points) ~= 'table' then error('Shift-point table unavailable; gearbox left unchanged.') end
  for index=gearbox.minGearIndex,gearbox.maxGearIndex do
    local point = points[index]
    if type(point) ~= 'table' then error('Missing controller shift points; reset the vehicle first.') end
    for _, field in ipairs({'lowShiftDownAV','highShiftDownAV','lowShiftUpAV','highShiftUpAV'}) do
      if not finite(point[field]) or point[field] < 0 then error('Unsupported controller shift points; gearbox left unchanged.') end
    end
  end
  return points
end

local function copyPoints(points)
  local out = {}
  for index, point in pairs(points) do out[index] = type(point)=='table' and copy(point) or point end
  return out
end

local function replacePoints(target, source)
  for index in pairs(target) do target[index]=nil end
  for index, point in pairs(copyPoints(source)) do target[index]=point end
end

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
  local points = controllerShiftPoints(gearbox)
  if not gearBase or gearBase.device ~= gearbox then
    gearBase = {device=gearbox, max=template, count=gearbox.gearCount,
      minRatio=gearbox.minGearRatio,maxRatio=gearbox.maxGearRatio,
      points=points,stockPoints=copyPoints(points)}
    for _, field in ipairs(fields) do gearBase[field] = copy(gearbox[field]) end
  end
  if points ~= gearBase.points then error('Shift controller changed; reset the vehicle before editing gears.') end
  local nextPoints = copyPoints(gearBase.stockPoints)
  for index in pairs(nextPoints) do if type(index)=='number' and index>0 then nextPoints[index]=nil end end
  for index=1,#ratios do nextPoints[index]=copy(gearBase.stockPoints[math.min(index,gearBase.max)]) end
  local staged, previous = {}, {}
  for _, field in ipairs(fields) do
    previous[field] = gearbox[field]
    staged[field] = copy(gearBase[field])
    for index in pairs(staged[field]) do if type(index)=='number' and index>0 then staged[field][index]=nil end end
    for index=1,#ratios do
      staged[field][index] = field == 'gearRatios' and ratios[index] or gearBase[field][math.min(index,gearBase.max)]
    end
  end
  -- The GE bridge requires a stopped vehicle; select neutral before resizing.
  gearbox:setGearIndex(0)
  local oldMax,oldCount,oldLow,oldHigh = gearbox.maxGearIndex,gearbox.gearCount,gearbox.minGearRatio,gearbox.maxGearRatio
  local oldPoints = copyPoints(points)
  local low, high = math.huge, 0
  for _, ratio in pairs(staged.gearRatios) do
    if ratio ~= 0 then low, high = math.min(low, math.abs(ratio)), math.max(high, math.abs(ratio)) end
  end
  local ok, err = pcall(function()
    replacePoints(points,nextPoints)
    for _, field in ipairs(fields) do gearbox[field]=staged[field] end
    gearbox.maxGearIndex,gearbox.gearCount = #ratios,#ratios+math.abs(gearbox.minGearIndex or -1)
    gearbox.minGearRatio,gearbox.maxGearRatio = low,high
    powertrain.calculateTreeInertia()
  end)
  if not ok then
    for _, field in ipairs(fields) do gearbox[field]=previous[field] end
    replacePoints(points,oldPoints)
    gearbox.maxGearIndex,gearbox.gearCount,gearbox.minGearRatio,gearbox.maxGearRatio = oldMax,oldCount,oldLow,oldHigh
    pcall(powertrain.calculateTreeInertia)
    error('Gearbox update failed; previous tables restored: '..tostring(err))
  end
  return 'Manual ratios applied. Sequential shifting is supported. Ordinary reset keeps the recipe when enabled; Full reset clears it.'
end

-- Use the same node/rest-length/commit API as beamstate.load. Scale physical
-- nodes in the car's own axes, not the scene object's purely visual transform.
local function dimensions(settings)
  local width, length, height = settings.width, settings.length, settings.height
  if not between(width, .5, 2) or not between(length, .5, 2) or not between(height, .5, 2) then error('Dimensions must be 0.5–2x on each axis.') end
  for _, method in ipairs({'getNodePosition','setNodePosition','getBeamRestLength','setBeamLength','beamIsBroken','commitLoad','getDirectionVector','getDirectionVectorUp'}) do
    if type(obj[method]) ~= 'function' then error('Physical resizing API unavailable: '..method) end
  end
  local function unit(p)
    if not finite(p.x) or not finite(p.y) or not finite(p.z) then error('Invalid vehicle orientation.') end
    local n = math.sqrt(p.x*p.x+p.y*p.y+p.z*p.z)
    if not between(n, .9, 1.1) then error('Invalid vehicle orientation.') end
    return {x=p.x/n,y=p.y/n,z=p.z/n}
  end
  local function dot(a,b) return a.x*b.x+a.y*b.y+a.z*b.z end
  local forward, up = unit(obj:getDirectionVector()), unit(obj:getDirectionVectorUp())
  if math.abs(dot(forward,up)) > .01 then error('Vehicle orientation axes are not orthogonal.') end
  local right = unit({x=forward.y*up.z-forward.z*up.y,y=forward.z*up.x-forward.x*up.z,z=forward.x*up.y-forward.y*up.x})
  for _, beam in pairs(v.data.beams or {}) do
    if obj:beamIsBroken(beam.cid) then error('Repair/reset the vehicle before resizing broken beams.') end
  end
  local base = dimensionBase
  if not base then
    base = {nodes={},beams={},bottom=math.huge}
    for _, node in pairs(v.data.nodes or {}) do
      local p = obj:getNodePosition(node.cid)
      if not finite(p.x) or not finite(p.y) or not finite(p.z) then error('Invalid node geometry.') end
      local point = {x=dot(p,right),y=dot(p,forward),z=dot(p,up)}
      base.nodes[node.cid] = point
      base.bottom = math.min(base.bottom,point.z)
    end
    if not next(base.nodes) then error('No physical nodes available.') end
    for _, beam in pairs(v.data.beams or {}) do
      local a,b = base.nodes[beam.id1],base.nodes[beam.id2]
      local rest = obj:getBeamRestLength(beam.cid)
      if not a or not b or not finite(rest) or rest <= 0 then error('Unsupported beam geometry.') end
      local dx,dy,dz = a.x-b.x,a.y-b.y,a.z-b.z
      local span = math.sqrt(dx*dx+dy*dy+dz*dz)
      if not finite(span) or span < 1e-9 then error('Cannot resize coincident beam endpoints.') end
      base.beams[#base.beams+1] = {cid=beam.cid,dx=dx,dy=dy,dz=dz,span=span,rest=rest}
    end
  end
  -- Precompute everything before the first mutation. Preserve beam preload,
  -- mass and the bottom of the captured car; never compound repeated applies.
  local positions, rests = {}, {}
  for cid,p in pairs(base.nodes) do
    local x,y,z = p.x*width,p.y*length,base.bottom+(p.z-base.bottom)*height
    local q = {x=right.x*x+forward.x*y+up.x*z,y=right.y*x+forward.y*y+up.y*z,z=right.z*x+forward.z*y+up.z*z}
    if not finite(q.x) or not finite(q.y) or not finite(q.z) then error('Scaled node would overflow.') end
    positions[cid] = vec3(q.x,q.y,q.z)
  end
  for _,b in ipairs(base.beams) do
    local rest = b.rest*math.sqrt((b.dx*width)^2+(b.dy*length)^2+(b.dz*height)^2)/b.span
    if not finite(rest) or rest <= 0 then error('Scaled beam would overflow.') end
    rests[b.cid] = rest
  end
  dimensionBase = base
  for cid,p in pairs(positions) do obj:setNodePosition(cid,p) end
  for cid,rest in pairs(rests) do obj:setBeamLength(cid,rest) end
  obj:commitLoad()
  return 'Experimental dimensions applied. Suspension/controllers may fight scaling. Ordinary reset keeps the recipe when enabled; Full reset clears it.'
end

local function stability()
  local esc = controller and controller.getController and controller.getController('esc')
  if not esc or not esc.isExisting or type(esc.toggleESCMode) ~= 'function' then error('This vehicle has no supported ESC controller.') end
  esc.toggleESCMode()
  return 'Cycled the vehicle ESC/traction-control mode. Check its in-game indicator.'
end

local function executeRaw(action, settings)
  if action == 'power' then return setPower(settings.enginePower)
  elseif action == 'fullReset' then
    if not obj or type(obj.requestReset) ~= 'function' then error('Physics reset API unavailable.') end
    if type(obj.getVelocity) ~= 'function' then error('Vehicle speed API unavailable; reset refused.') end
    local speed=obj:getVelocity():length()
    if not finite(speed) or speed>1 then error('Vehicle started moving; reset refused.') end
    M.restoreForPolicy()
    return 'ABM vehicle settings cleared; physics reset requested. World gravity was not changed.'
  elseif action == 'rpm' then return setRPM(settings.maxRPM)
  elseif action == 'clutch' then return protectClutch(settings.clutchIndestructible)
  elseif action == 'tuning' then return tuning(settings)
  elseif action == 'engine' then return engineStrength(settings)
  elseif action == 'dimensions' then return dimensions(settings)
  elseif action == 'handling' then return apply(settings)
  elseif action == 'gears' then return gears(settings.ratios)
  elseif action == 'stability' then return stability()
  end
  error('Unsupported vehicle tuning action.')
end

local persistentActions={power=true,tuning=true,engine=true,rpm=true,clutch=true,handling=true,gears=true,dimensions=true}
local function execute(action,settings)
  if action=='persistence' then
    if type(settings.enabled)~='boolean' then error('Reset persistence must be on or off.') end
    resetPersistence=settings.enabled
    if not resetPersistence then recipe={}; restorePending=false; restoreStatus='off' else restoreStatus='idle' end
    return 'Reset persistence '..(resetPersistence and 'ON for future accepted edits.' or 'OFF; saved reset recipe cleared.')
  end
  if restorePending and action~='fullReset' then error('Reset restoration in progress; wait or use Full reset.') end
  local message=executeRaw(action,settings)
  if resetPersistence and persistentActions[action] then
    recipe[action]=copy(settings)
    if settings.ratios then recipe[action].ratios=copy(settings.ratios) end
    if action=='tuning' then recipe.power=nil end
    restoreStatus,restoreError='saved',''
  end
  return message
end
M.execute = execute
local authorizationContext
M.authorize = function(expiresAt,context,allowed)
  if type(context)~='string' or #context>800 or not finite(expiresAt) or expiresAt>os.time()+3 then return end
  if authorizationContext and authorizationContext~=context then recipe={}; restorePending=false; restoreStatus='context changed' end
  authorizationContext=context
  authorizedUntil=allowed==true and expiresAt or 0
  if not allowed then recipe={}; restorePending=false; restoreStatus='policy blocked' end
end
M.target = function() return instance..':'..generation end
M.executeTarget = function(action, settings, target, expiresAt, revision, client)
  if target ~= M.target() then error('Vehicle instance changed; select it again.') end
  if not finite(expiresAt) or os.time() >= expiresAt then error('Vehicle request expired; it was not applied.') end
  if type(client) ~= 'string' or #client > 80 or #client == 0 or not between(revision,1,2147483647) or revision % 1 ~= 0 then error('Invalid workshop intent.') end
  if revisions[client] and revision <= revisions[client] then error('Superseded workshop intent.') end
  local count=0; for _ in pairs(revisions) do count=count+1 end
  if not revisions[client] and count >= 32 then error('Too many workshop clients; reset the vehicle.') end
  revisions[client] = revision
  return execute(action,settings)
end
M.readback = function()
  local out = {contract=1,generation=M.target(),engines={},gears={},persistence=resetPersistence,
    restoreStatus=restoreStatus,restoreError=restoreError,restoring=restorePending,clutchProtected=next(clutchBases)~=nil}
  if acceptedPower then out.enginePower = acceptedPower end
  for name, device in pairs(powertrain and powertrain.getDevicesByCategory and powertrain.getDevicesByCategory('engine') or {}) do
    if #out.engines >= 8 then break end
    local item = {name=tostring(name):sub(1,80)}
    if finite(device.maxRPM) then item.maxRPM=device.maxRPM end
    if finite(device.revLimiterRPM) then item.revLimiterRPM=device.revLimiterRPM end
    if finite(device.revLimiterRPM) and pcall(rpmDevices) then item.rpmCeiling=rpmCeiling(device); item.idleRPM=device.idleRPM end
    if finite(device.maxPhysicalAV) then item.physicalRPM=device.maxPhysicalAV*30/math.pi end
    out.engines[#out.engines+1]=item
  end
  local devices = powertrain and powertrain.getDevicesByType and powertrain.getDevicesByType('manualGearbox') or {}
  local gearbox, count=nil,0
  for _, device in pairs(devices) do gearbox,count=device,count+1 end
  if count == 1 and type(gearbox.gearRatios)=='table' and between(gearbox.minGearIndex,-16,0) and between(gearbox.maxGearIndex,1,256) then
    out.gearbox=tostring(gearbox.name or 'manualGearbox'):sub(1,80)
    for index=gearbox.minGearIndex,gearbox.maxGearIndex do
      local ratio=gearbox.gearRatios[index]
      if finite(ratio) then out.gears[#out.gears+1]={index=index,ratio=ratio} end
    end
  end
  return out
end
M.onReset = function()
  generation = generation+1
  revisions = {}
  acceptedPower = nil
  for _,device in pairs(powertrain and powertrain.getDevicesByCategory and powertrain.getDevicesByCategory('engine') or {}) do
    if device.beamdeckBase then device.torqueCurve=copy(device.beamdeckBase) end
    if device.beamdeckIdle~=nil then device.idleTorque=device.beamdeckIdle end
  end
  dimensionBase = nil -- The physics reset restores stock node geometry.
  for device,base in pairs(rpmBases) do for _,field in ipairs(rpmFields) do device[field]=base[field] end end
  rpmBases={}
  for device,base in pairs(clutchBases) do
    if device.updateGFX==base.wrapper then device.updateGFX=base.updateGFX end
    if device.applyDeformGroupDamage==protected then device.applyDeformGroupDamage=base.damage end
  end
  clutchBases={}
  for device, base in pairs(engineBases) do restoreEngine(device, base) end
  engineBases = {}
  -- Vehicle extensions reset before powertrain/controllers. Restore dimensions
  -- first so their stock-ratio reset never indexes an expanded gearbox.
  if gearBase then
    local device = gearBase.device
    for _, field in ipairs(gearFields) do device[field] = copy(gearBase[field]) end
    device.maxGearIndex, device.gearCount = gearBase.max, gearBase.count
    device.minGearRatio, device.maxGearRatio = gearBase.minRatio, gearBase.maxRatio
    replacePoints(gearBase.points,gearBase.stockPoints)
    gearBase = nil
  end
  for _, hydro in pairs(hydros and hydros.hydros or {}) do
    if hydro.abmInputFactor then hydro.inputFactor, hydro.abmInputFactor = hydro.abmInputFactor, nil end
  end
  restorePending=resetPersistence and next(recipe)~=nil
  restoreFrames,restoreAge=0,0
  restoreStatus=restorePending and 'waiting for reset' or 'idle'
end
M.updateGFX=function(dt)
  if not restorePending then return end
  restoreFrames,restoreAge=restoreFrames+1,restoreAge+(finite(dt) and math.max(0,dt) or 0)
  if restoreAge>10 then restorePending=false; restoreStatus='suspended'; restoreError='Reset not ready within 10 seconds. Reapply manually.'; return end
  if restoreFrames<2 or restoreAge<.1 or os.time()>=authorizedUntil then return end
  if recipe.gears or recipe.dimensions then
    if type(obj.getVelocity)~='function' or not finite(obj:getVelocity():length()) or obj:getVelocity():length()>1 then return end
  end
  -- Apply exactly once to fresh post-reset baselines. A failed adapter suspends
  -- the entire recipe rather than repeatedly deforming geometry or resetting.
  restorePending=false
  local ok,message=pcall(function()
    for _,action in ipairs({'tuning','power','engine','rpm','clutch','handling','gears','dimensions'}) do
      if recipe[action] then executeRaw(action,recipe[action]) end
    end
  end)
  restoreStatus=ok and 'restored' or 'suspended'
  restoreError=ok and '' or tostring(message):sub(1,256)
end
M.capabilities = function()
  local caps = {power=false, engine=false, grip=false, brakes=false, steering=false, gears=false,
    dimensions=false, deformation=false, stability=false, ignite=false, extinguish=false, deflate=false}
  for _, device in pairs(powertrain and powertrain.getDevicesByCategory and powertrain.getDevicesByCategory('engine') or {}) do
    if type(device.torqueCurve) == 'table' and next(device.torqueCurve) then caps.power = true end
  end
  local engines = powertrain and powertrain.getDevicesByType and powertrain.getDevicesByType('combustionEngine') or {}
  caps.engine = next(engines) ~= nil
  caps.rpm=pcall(rpmDevices)
  caps.clutch=pcall(clutches)
  for _, device in pairs(engines) do
    for _, field in ipairs(strengthFields) do if not finite(device[field]) then caps.engine = false end end
    for _, method in ipairs(protectionMethods) do if type(device[method]) ~= 'function' then caps.engine = false end end
    if not finite(device.maxPhysicalAV) or not device.thermals or type(device.thermals.updateGFX) ~= 'function' then caps.engine = false end
  end
  for id, wheel in pairs(v.data.wheels or {}) do
    local state = wheels and wheels.wheels and wheels.wheels[id]
    if not (state and (state.isTireDeflated or state.isBroken)) and type(wheel.treadNodes) == 'table'
      and next(wheel.treadNodes) and type(obj.setNodeFrictionSlidingCoefs) == 'function' then caps.grip = true end
  end
  caps.brakes = wheels ~= nil and type(wheels.scaleBrakeTorque) == 'function'
  for _, hydro in pairs(hydros and hydros.hydros or {}) do
    if (hydro.inputSource == 'steering_input' or hydro.inputSource == 'steering') and finite(hydro.inputFactor) then caps.steering = true end
  end
  local devices = powertrain and powertrain.getDevicesByType and powertrain.getDevicesByType('manualGearbox') or {}
  local gearbox, amount
  amount = 0
  for _, device in pairs(devices) do gearbox, amount = device, amount+1 end
  if amount == 1 and type(gearbox.setGearIndex) == 'function' and type(powertrain.calculateTreeInertia) == 'function' then
    caps.gears = pcall(function()
      controllerShiftPoints(gearbox)
      for _, field in ipairs(gearFields) do
        if type(gearbox[field]) ~= 'table' or gearbox[field][gearbox.maxGearIndex] == nil then error('Unsupported gearbox') end
      end
    end)
  end
  caps.dimensions = next(v.data.nodes or {}) ~= nil and next(v.data.beams or {}) ~= nil
  for _, method in ipairs({'getNodePosition','setNodePosition','getBeamRestLength','setBeamLength','beamIsBroken','commitLoad','getDirectionVector','getDirectionVectorUp'}) do
    if type(obj[method]) ~= 'function' then caps.dimensions = false end
  end
  if caps.dimensions then
    for _, beam in pairs(v.data.beams) do if obj:beamIsBroken(beam.cid) then caps.dimensions = false; break end end
  end
  caps.deformation = next(v.data.beams or {}) ~= nil and type(obj.setBeamDeform) == 'function' and type(obj.setBeamStrength) == 'function'
  local esc = controller and controller.getController and controller.getController('esc')
  caps.stability = esc ~= nil and esc.isExisting == true and type(esc.toggleESCMode) == 'function'
  caps.ignite = fire ~= nil and type(fire.igniteVehicle) == 'function'
  caps.extinguish = fire ~= nil and type(fire.extinguishVehicle) == 'function'
  caps.deflate = caps.grip and beamstate ~= nil and type(beamstate.deflateTires) == 'function'
  return caps
end
M.restoreForPolicy = function()
  recipe,restorePending,restoreStatus,restoreError={},false,'cleared',''
  authorizedUntil=0
  -- Restore ABM-owned values before the normal physics reset. Reset alone does
  -- not rebuild a modified torque curve or the manual gearbox's private tables.
  tuning({enginePower=1,damageImmunity=false})
  apply({tireGrip=1,slidingGrip=1,brakePower=1,steeringResponse=1})
  M.onReset()
  acceptedPower = nil
  obj:requestReset(RESET_PHYSICS)
end
return M
