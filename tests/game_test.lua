-- Strict public-API/socket mocks: no game process or real socket is contacted.
local sent, objects, inbound, commands = {}, {}, {}, {}
local peer, closed
local udp = {
  settimeout=function(_,value) assert(value==0) end,
  setpeername=function(_,ip,port) assert(ip=='127.0.0.1' and port==44444); peer=true end,
  send=function(_,bytes,...) assert(peer and select('#',...)==0); sent[#sent+1]=bytes end,
  receive=function() assert(peer); return table.remove(inbound,1) end,
  close=function() closed=true end
}
package.preload.socket=function() return {udp=function() return udp end} end
setExtensionUnloadMode=function() end
log=function() end
beamng_versionb='0.40.5'
getMissionFilename=function() return '/levels/west_coast_usa/info.json' end
VariableRegistry={get=function() return 90 end}
simTimeAuthority={getPause=function() return false end}
FS={getGamePath=function() return '/game' end,getUserPath=function() return '/user' end,
  getFileOriginInfo=function() return {archivePath='/game/content/vehicles/etk800.zip'} end,
  getFileRealPath=function(_,path) return '/game'..path end}
be={getObjectCount=function() return 1 end}
local vehicleCommands={}
local speed=25
local vehicle={getID=function() return 42 end,getJBeamFilename=function() return 'etk800' end,
  getVelocity=function() return {length=function() return speed end} end,
  getPosition=function() return {x=0,y=0,z=0} end,
  getDirectionVector=function() return {x=1,y=0,z=0} end,
  queueLuaCommand=function(_,lua)
    if lua:find('local e = electrics.values',1,true) then vehicleCommand=lua
    else vehicleCommands[#vehicleCommands+1]=lua end
  end}
quatFromDir=function() return {w=1,x=0,y=0,z=0} end
spawn={safeTeleport=function(veh,pos,rot,a,b,c,d,repair) assert(veh==vehicle and repair==true); return true end,
  spawnNewVehicle=function() end}
local envGravity
core_environment={setGravity=function(g) envGravity=g end}
getPlayerVehicle=function() return vehicle end
local last
jsonEncode=function(data) last=data; objects[#objects+1]=data; return 'encoded' end
jsonDecode=function(bytes) return commands[bytes] end
local m=dofile('game/lua/ge/extensions/beamdeck.lua')
m.onExtensionLoaded(); assert(peer)
m.onUpdate(.06)
assert(last.multiplayer==false and last.telemetry.speed==25 and last.bridgeVersion=='0.4.0')
assert(last.map=='west_coast_usa' and vehicleCommand)
m.receiveVehicle(5,{rpm=99}); m.onUpdate(.06); assert(last.telemetry.rpm==nil)
m.receiveVehicle(42,{rpm=3200,fuel=.7}); m.onUpdate(.06); assert(last.telemetry.rpm==3200)
MPCoreNetwork={isMPSession=function() return true end,getCurrentServer=function() return {name='<b>Server</b>'} end}
MPVehicleGE={getPlayers=function() return {
  {name='<img src="https://example.invalid">',isLocal=true,ping=-1,vehicles={IDs={a=true}}},
  {name='Other',ping=45,vehicles={IDs={b=true,c=true}}}} end,
  getVehicles=function() return {a=true,b=true,c=true} end}
m.onUpdate(.06)
assert(last.multiplayer and last.playerCount==2 and last.mpVehicleCount==3 and last.ping==nil)
assert(last.players[2].ping==45)
MPCoreNetwork.isMPSession=function() return false end
local serial=0
local function request(action,values,session)
  serial=serial+1
  local id='request'..serial
  local c=values or {}
  c.action,c.requestId,c.source,c.schema,c.sessionId=action,id,'beamdeck-control',1,session or last.sessionId
  commands[id]=c; inbound[#inbound+1]=id
  for _=1,100 do
    m.onUpdate(.06)
    for i=#objects,1,-1 do if objects[i].requestId==id and (objects[i].kind=='result' or objects[i].complete) then return objects[i] end end
  end
end
local spawns,replaces,entered=0,0,0
local configurations={
  base={Configuration='Factory',Name='800 Factory',pcFilename='/vehicles/etk800/base.pc',preview='/vehicles/etk800/base.jpg',Source='BeamNG - Official'},
  ['Racely (M)']={Configuration='Racely (M)',pcFilename='/vehicles/etk800/Racely (M).pc',preview='/vehicles/etk800/Racely (M).jpg',Source='Custom'}
}
core_vehicles={
  getModelList=function() return {models={etk800={key='etk800',Name='800',Brand='ETK',Type='Car',preview='/vehicles/etk800/default.jpg'}}} end,
  getModel=function(model) assert(model=='etk800'); return {configs=configurations} end,
  spawnNewVehicle=function(model,options) assert(model=='etk800' and type(options)=='table' and options.config=='/vehicles/etk800/Racely (M).pc');spawns=spawns+1 end,
  replaceVehicle=function(model,options) assert(model=='etk800' and type(options)=='table' and options.config=='/vehicles/etk800/base.pc');replaces=replaces+1 end
}
local response=request('catalog')
assert(response.kind=='catalog' and response.complete and response.items[1].id=='etk800' and response.items[1].preview.path)
response=request('configs',{model='etk800'})
assert(response.kind=='configs' and #response.items==2 and response.items[1].name)
local custom
for _,c in ipairs(response.items) do if c.key=='Racely (M)' then custom=c end end
assert(custom and custom.custom and custom.preview.path=='/vehicles/etk800/Racely (M).jpg')
assert(not request('configs',{model='../bad'}).ok)
speed=0
response=request('spawn',{model='etk800',config='Racely (M)'})
assert(response.ok and spawns==1)
assert(request('replace',{model='etk800',config='base'}).ok and replaces==1)
assert(not request('spawn',{model='etk800',config='../inject'}).ok and spawns==1)
assert(not request('spawn',{model='etk800',config='missing'}).ok and spawns==1)
speed=25
assert(not request('spawn',{model='etk800',config='Racely (M)'}).ok and spawns==1)
speed=0
assert(request('spawn',{model='etk800',config='Racely (M)'},'old-session')==nil and spawns==1)
MPCoreNetwork.isMPSession=function() return true end
MPVehicleGE.isOwn=function() return false end
be.getObjectByID=function() return vehicle end
be.enterVehicle=function() entered=entered+1 end
assert(not request('switch',{vehicleId=42}).ok and entered==0)
assert(not request('replace',{model='etk800',config='base'}).ok and replaces==1)
MPVehicleGE.isOwn=function() return true end
assert(request('switch',{vehicleId=42}).ok and entered==1)
local mods={multiplayerbeammp={active=false}}
core_modmanager={getMods=function() return mods end,activateMod=function(name) assert(name=='multiplayerbeammp');mods[name].active=true end}
assert(request('enableBeamMP').ok and mods.multiplayerbeammp.active)
assert(not request('runLua',{code='1'}).ok)
-- Tuning: the cheat channel validates bounds, reaches the vehicle Lua and the
-- environment API, and re-applies itself on snapshot ticks.
vehicleCommands={}
assert(not request('tuning',{enginePower=5}).ok)
assert(not request('tuning',{gravity=-40}).ok)
assert(not request('tuning',{damageImmunity='yes'}).ok)
assert(request('tuning',{enginePower=1.5,gravity=-3.71,damageImmunity=true}).ok)
assert(#vehicleCommands>=1 and vehicleCommands[1]:find('beamdeckBase',1,true) and vehicleCommands[1]:find('switch',1,true)==nil)
assert(vehicleCommands[1]:find('1.5',1,true) and vehicleCommands[1]:find('true',1,true))
assert(envGravity==-3.71)
-- Execute the queued code, rather than merely checking its text. Category
-- results may be keyed by device name, and repeated applies must not compound.
local engine={torqueCurve={[1000]=200,[2000]=300},idleTorque=50}
local limits={}
local tuningEnv=setmetatable({powertrain={getDevicesByCategory=function() return {mainEngine=engine} end},
  v={data={beams={{cid=4,beamDeform=100,beamStrength=200}}}},
  obj={setBeamDeform=function(_,cid,value) limits.deform=value end,
    setBeamStrength=function(_,cid,value) limits.strength=value end}}, {__index=_G})
local tuneCode=assert(loadstring(vehicleCommands[1])); setfenv(tuneCode,tuningEnv)
tuneCode(); tuneCode()
assert(engine.torqueCurve[1000]==300 and limits.deform==1e30)
vehicleCommands={}
assert(request('tuning',{enginePower=1,gravity=-9.81,damageImmunity=false}).ok)
tuneCode=assert(loadstring(vehicleCommands[1])); setfenv(tuneCode,tuningEnv); tuneCode()
assert(engine.torqueCurve[1000]==200 and limits.deform==100 and limits.strength==200)
vehicleCommands={}
m.onUpdate(4) -- next snapshot tick re-applies the cheat to the same vehicle
assert(vehicleCommands[1] and vehicleCommands[1]:find('beamdeckBase',1,true))
-- Car actions: repair requires a stopped vehicle, the rest just fire.
speed=25
assert(not request('repair').ok)
speed=0
vehicleCommands={}
assert(request('repair').ok and vehicleCommands[1]==nil)
assert(request('ignite').ok and vehicleCommands[1]=='fire.igniteVehicle()')
assert(request('extinguish').ok and vehicleCommands[2]=='fire.extinguishVehicle()')
assert(request('deflate').ok and vehicleCommands[3]=='beamstate.deflateTires()')
assert(not request('launchMissiles').ok)
-- Verify multi-frame configurations are chunked, complete and never silently lost.
for i=1,60 do configurations['config'..i]={Configuration='Config '..i,pcFilename='/vehicles/etk800/config'..i..'.pc'} end
response=request('configs',{model='etk800'})
assert(response.complete and response.chunk==5)
local collected
local env={electrics={values={rpm=3200,abs=true,running=true,turboBoost=14.504}},
  powertrain={getDevice=function() return {maxRPM=7200} end},sensors={gx2=9.81,gy2=0},
  obj={getID=function() return 42 end,queueGameEngineLua=function() end},
  serialize=function(data) collected=data;return '{}' end,tostring=tostring,type=type}
local compiled=assert(loadstring(vehicleCommand));setfenv(compiled,env);compiled()
assert(collected.abs and collected.engineRunning and collected.boost==1)
getPlayerVehicle=function() return nil end
MPCoreNetwork.isMPSession=function() return false end
m.onUpdate(2);assert(not last.multiplayer and next(last.telemetry)==nil and last.players==nil)
m.onExtensionUnloaded();assert(closed)
print('Game bridge tests passed (configs, exact spawn options, sessions, ownership, mod activation, chunks, socket contract)')
