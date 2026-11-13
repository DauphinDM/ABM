(function () {
  'use strict';

  var snapshot = null, stateName = 'unpaired', deviceName = '', sequence = 0;
  var timer = null, polling = false, page = 'drive';
  var selectedModel = '', selectedConfig = '', garageKey = '', sessionKey = '';

  function id(name) { return document.getElementById(name); }
  function setText(name, value) { var node = id(name); if (node) node.textContent = value; }
  function show(name) {
    id('pairView').hidden = name !== 'pair';
    id('pendingView').hidden = name !== 'pending';
    id('appView').hidden = name !== 'app';
  }
  function message(text) { setText('actionMessage', text || ''); }
  function pairMessage(text) { setText('pairMessage', text || ''); }
  function pendingMessage(text) { setText('pendingMessage', text || ''); }
  function finite(value) {
    return value !== null && value !== undefined && value !== '' && isFinite(Number(value));
  }
  function format(value, digits) {
    return finite(value) ? Number(value).toFixed(digits) : '--';
  }
  function prop(object, key, fallback) {
    return object && object[key] !== undefined && object[key] !== null ? object[key] : fallback;
  }

  function api(method, path, body, done) {
    var xhr = new XMLHttpRequest();
    xhr.open(method, path, true);
    xhr.setRequestHeader('X-ABM-Web', '1');
    if (body !== null) xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.onreadystatechange = function () {
      if (xhr.readyState !== 4) return;
      var data = null;
      try { data = JSON.parse(xhr.responseText); } catch (ignore) {}
      done(xhr.status, data);
    };
    xhr.send(body === null ? null : JSON.stringify(body));
  }

  function schedule(delay) {
    if (timer) clearTimeout(timer);
    timer = setTimeout(poll, delay || 1000);
  }

  function poll() {
    if (polling) return;
    polling = true;
    api('GET', '/api/state', null, function (code, data) {
      polling = false;
      if (code === 200 && data && data.status === 'approved') {
        stateName = 'approved';
        deviceName = data.device || deviceName;
        snapshot = data.snapshot || null;
        if (data.seq !== undefined && finite(data.seq) && Number(data.seq) > sequence) sequence = Number(data.seq);
        if (data.trusted && data.trusted.id && data.trusted.token) saveTrusted(data.trusted);
        show('app');
        render();
      } else if (code === 200 && data && data.status === 'pending') {
        stateName = 'pending';
        deviceName = data.device || deviceName;
        setText('pendingName', deviceName || 'this browser');
        show('pending');
        pendingMessage('Waiting for approval. This page checks again automatically.');
      } else if (code === 401) {
        stateName = 'unpaired';
        snapshot = null;
        show('pair');
        updateResume();
      } else {
        message('Connection lost. Retrying...');
        schedule(2000);
        return;
      }
      schedule(1000);
    });
  }

  function extractSecret(raw) {
    var text = String(raw || '').replace(/^\s+|\s+$/g, '');
    if (/^[0-9a-f]{64}$/.test(text)) return text;
    if (text.indexOf('abm://pair/') !== 0) return '';
    var payload = text.slice(11).replace(/-/g, '+').replace(/_/g, '/');
    while (payload.length % 4) payload += '=';
    try {
      var invite = JSON.parse(atob(payload));
      if (invite && invite.v === 1 && /^[0-9a-f]{64}$/.test(invite.secret)) return invite.secret;
    } catch (ignore) {}
    return '';
  }

  function loadTrusted() {
    try {
      var value = JSON.parse(localStorage.getItem('abm_web_trusted') || 'null');
      if (value && /^[0-9a-f]{32}$/.test(value.id) && /^[0-9a-f]{64}$/.test(value.token)) return value;
    } catch (ignore) {}
    return null;
  }
  function saveTrusted(value) {
    try { localStorage.setItem('abm_web_trusted', JSON.stringify(value)); } catch (ignore) {}
  }
  function clearTrusted() {
    try { localStorage.removeItem('abm_web_trusted'); } catch (ignore) {}
  }
  function updateResume() {
    var button = id('resumeButton');
    if (button) button.hidden = !loadTrusted();
  }

  function pair() {
    var secret = extractSecret(id('link').value);
    var name = String(id('device').value || '').replace(/^\s+|\s+$/g, '');
    if (!secret) { pairMessage('Paste a valid, unexpired pairing link or its 64-character secret.'); return; }
    if (!name || name.length > 64) { pairMessage('Enter a browser name between 1 and 64 characters.'); return; }
    id('pairButton').disabled = true;
    pairMessage('Requesting approval...');
    api('POST', '/api/pair', { secret: secret, device: name }, function (code, data) {
      id('pairButton').disabled = false;
      id('link').value = '';
      if (code === 200 && data && (data.status === 'pending' || data.status === 'approved')) {
        stateName = data.status;
        deviceName = name;
        pairMessage('');
        if (data.status === 'approved') poll();
        else {
          setText('pendingName', name);
          show('pending');
          pendingMessage('Waiting for approval in PC ABM > Connect.');
          schedule(700);
        }
      } else {
        pairMessage((data && (data.error || data.message)) || 'Pairing was refused or the link expired. Enable access again on the PC for a fresh link.');
      }
    });
  }

  function resume() {
    var saved = loadTrusted();
    if (!saved) { updateResume(); return; }
    id('resumeButton').disabled = true;
    pairMessage('Resuming saved browser...');
    api('POST', '/api/resume', saved, function (code, data) {
      id('resumeButton').disabled = false;
      if (code === 200 && data && data.status === 'approved') {
        pairMessage('');
        poll();
      } else {
        clearTrusted();
        updateResume();
        pairMessage((data && (data.error || data.message)) || 'Saved browser access was revoked. Pair again.');
      }
    });
  }

  function mutable() {
    if (stateName !== 'approved' || !snapshot) return false;
    var state = snapshot.state || {};
    var telemetry = state.telemetry || {};
    var speed = Number(telemetry.speed);
    return !snapshot.busy && !!state.connected && !!state.controlsAvailable && !!state.vehicleOwned &&
      !!state.sandboxAllowed && !state.joining && !state.policyCleanup &&
      finite(telemetry.speed) && speed >= 0 && speed <= 1;
  }
  function spawnable() {
    if (stateName !== 'approved' || !snapshot) return false;
    var state = snapshot.state || {};
    return !snapshot.busy && !!state.connected && !!state.controlsAvailable && !!state.sandboxAllowed &&
      !state.joining && !state.policyCleanup && !!state.map &&
      (state.vehicleId === null || state.vehicleId === undefined || mutable());
  }
  function browseable() {
    if (stateName !== 'approved' || !snapshot) return false;
    var state = snapshot.state || {};
    return !!state.connected && !!state.controlsAvailable && !snapshot.busy;
  }

  function command(action, args) {
    if (stateName !== 'approved' || !snapshot) { message('Not connected. Nothing was queued.'); return; }
    sequence += 1;
    message('Sending ' + action + '...');
    api('POST', '/api/command', {
      seq: sequence, action: action, args: args || {}, target: snapshot.target || {}
    }, function (code, data) {
      if (code === 200 && data && data.ok) {
        message(data.message || 'Request sent.');
        schedule(250);
      } else if (code === 401) {
        stateName = 'unpaired'; snapshot = null; show('pair'); updateResume();
        pairMessage('The browser session ended. Pair again.');
      } else {
        message((data && (data.error || data.message)) || 'The PC rejected this request.');
      }
    });
  }

  function render() {
    var state = snapshot ? (snapshot.state || {}) : {};
    var telemetry = state.telemetry || {};
    var connected = stateName === 'approved' && !!state.connected;
    var badge = id('badge');
    badge.className = 'badge ' + (connected ? 'on' : 'off');
    badge.textContent = connected ? 'GAME CONNECTED' : stateName === 'approved' ? 'PC CONNECTED' : 'NOT PAIRED';
    setText('connectionText', deviceName ? ('Browser: ' + deviceName) : 'Browser connected');
    message(snapshot && snapshot.actionMessage ? snapshot.actionMessage : '');
    setText('speed', finite(telemetry.speed) ? format(Number(telemetry.speed) * 3.6, 0) : '--');
    setText('gear', prop(telemetry, 'gear', '--'));
    setText('rpm', format(telemetry.rpm, 0));
    setText('fuel', finite(telemetry.fuel) ? format(Number(telemetry.fuel) * 100, 0) : '--');
    setText('coolant', format(telemetry.waterTemp, 0));
    setText('latency', format(state.ping, 0));

    var canChange = mutable();
    setText('workshopNote', canChange
      ? 'Stopped, owned vehicle. Every request is checked again by the PC and game.'
      : 'Connect a live game and stop an owned vehicle on a permitted server to change settings.');
    var actions = document.querySelectorAll('[data-action]');
    for (var i = 0; i < actions.length; i++) {
      var button = actions[i], action = button.getAttribute('data-action'), valid = canChange;
      if (action === 'power') valid = canChange && finite(id('power').value) &&
        Number(id('power').value) >= 1 && Number(id('power').value) <= 2000;
      if (action === 'rpm') valid = canChange && finite(id('rpmLimit').value) &&
        Number(id('rpmLimit').value) >= 100 && Number(id('rpmLimit').value) <= 30000;
      if (action === 'gears') valid = canChange && String(id('ratios').value || '').replace(/^\s+|\s+$/g, '').length > 0 &&
        String(id('ratios').value).length <= 4000;
      button.disabled = !valid;
    }
    id('catalogButton').disabled = !browseable();
    id('backModels').disabled = !browseable();
    id('spawnButton').disabled = !(spawnable() && selectedConfig);
    id('replaceButton').disabled = !(canChange && selectedConfig);
    id('resetTrip').disabled = !canChange;
    setText('browserInfo', 'Approved browser session for ' + (deviceName || 'this device') +
      '. Commands are limited to the fixed allowlist and require a live, permitted game.');
    renderGarage(canChange);
    renderSession();
  }

  function clear(node) { while (node.firstChild) node.removeChild(node.firstChild); }
  function addItem(container, title, subtitle, attributes) {
    var button = document.createElement('button');
    button.className = 'item';
    var name = document.createElement('span');
    name.textContent = title;
    button.appendChild(name);
    if (subtitle) {
      var sub = document.createElement('span');
      sub.className = 'sub';
      sub.textContent = subtitle;
      button.appendChild(sub);
    }
    for (var key in attributes) {
      if (Object.prototype.hasOwnProperty.call(attributes, key)) button.setAttribute(key, attributes[key]);
    }
    container.appendChild(button);
    return button;
  }

  function renderGarage(canChange) {
    var list = id('garageList');
    var catalog = snapshot && snapshot.catalog ? snapshot.catalog : [];
    var configs = snapshot && snapshot.configs ? snapshot.configs : [];
    var configsModel = snapshot ? snapshot.configsModel : '';
    var items = selectedModel ? (configsModel === selectedModel ? configs : []) : catalog;
    var key = selectedModel + '|' + selectedConfig + '|' + configsModel + '|' + JSON.stringify(items);
    id('backModels').hidden = !selectedModel;
    id('catalogButton').textContent = selectedModel ? 'Refresh models' : 'Refresh catalog';
    setText('garageNote', selectedModel
      ? (configsModel === selectedModel ? 'Select a configuration, then spawn or replace.' : 'Loading configurations...')
      : 'Select a vehicle model to load its configurations.');
    if (key === garageKey) return;
    garageKey = key;
    clear(list);
    if (!items.length) {
      var empty = document.createElement('p');
      empty.className = 'empty';
      empty.textContent = selectedModel ? 'No configurations loaded.' : 'No vehicles loaded. Connect a game and refresh.';
      list.appendChild(empty);
      return;
    }
    for (var i = 0; i < items.length; i++) {
      var item = items[i];
      var title = item.name || item.id || 'Unnamed';
      var sub = item.id || '';
      var selected = selectedModel && item.id === selectedConfig;
      var button = addItem(list, title, sub, selectedModel ? { 'data-config': item.id || '' } : { 'data-model': item.id || '' });
      if (selected) button.className += ' selected';
    }
  }

  function renderSession() {
    var state = snapshot ? (snapshot.state || {}) : {};
    setText('serverName', state.server ? state.server : (state.connected ? 'Single player' : 'No live game'));
    var players = state.players || [], vehicles = state.activeVehicles || [];
    var key = JSON.stringify(players) + '|' + JSON.stringify(vehicles) + '|' + mutable();
    if (key === sessionKey) return;
    sessionKey = key;
    var playerList = id('players');
    clear(playerList);
    if (!players.length) {
      var none = document.createElement('p');
      none.className = 'empty';
      none.textContent = 'No players reported by the game.';
      playerList.appendChild(none);
    } else {
      for (var i = 0; i < players.length; i++) {
        var player = players[i];
        playerList.appendChild(document.createElement('div')).className = 'item';
        var row = playerList.lastChild;
        var name = document.createElement('span');
        name.textContent = player.rawName || player.name || 'Unknown player';
        row.appendChild(name);
        var sub = document.createElement('span');
        sub.className = 'sub';
        sub.textContent = 'Session ID: ' + prop(player, 'sessionPlayerId', '--');
        row.appendChild(sub);
      }
    }
    var switchList = document.getElementById('ownedVehicles');
    if (!switchList) {
      switchList = document.createElement('div');
      switchList.id = 'ownedVehicles';
      switchList.className = 'list';
      playerList.parentNode.insertBefore(switchList, playerList.nextSibling);
    }
    clear(switchList);
    for (var j = 0; j < vehicles.length; j++) {
      var vehicle = vehicles[j];
      if (!vehicle.owned) continue;
      var switchButton = addItem(switchList, 'Switch to ' + (vehicle.name || vehicle.id), 'Owned vehicle', { 'data-switch': String(vehicle.id) });
      switchButton.disabled = !mutable();
    }
  }

  function submitAction(action, button) {
    if (action === 'power') command('power', { value: Number(id('power').value) / 100 });
    else if (action === 'rpm') command('rpm', { value: Number(id('rpmLimit').value) });
    else if (action === 'clutch') command('clutch', { enabled: button.getAttribute('data-bool') === 'true' });
    else if (action === 'gears') command('gears', { ratios: String(id('ratios').value) });
    else command(action, {});
  }

  function closestData(node, name) {
    while (node && node !== document) {
      if (node.getAttribute && node.getAttribute(name) !== null) return node;
      node = node.parentNode;
    }
    return null;
  }

  document.addEventListener('click', function (event) {
    var node = event.target;
    var actionNode = closestData(node, 'data-action');
    if (actionNode) { submitAction(actionNode.getAttribute('data-action'), actionNode); return; }
    var modelNode = closestData(node, 'data-model');
    if (modelNode) {
      selectedModel = modelNode.getAttribute('data-model');
      selectedConfig = '';
      garageKey = '';
      command('configs', { model: selectedModel });
      return;
    }
    var configNode = closestData(node, 'data-config');
    if (configNode) { selectedConfig = configNode.getAttribute('data-config'); render(); return; }
    var switchNode = closestData(node, 'data-switch');
    if (switchNode && mutable()) {
      command('switch', { id: Number(switchNode.getAttribute('data-switch')) });
      return;
    }
    var tabNode = closestData(node, 'data-page');
    if (tabNode) {
      page = tabNode.getAttribute('data-page');
      var tabs = document.querySelectorAll('[data-page]');
      for (var i = 0; i < tabs.length; i++) tabs[i].className = tabs[i] === tabNode ? 'active' : '';
      var pages = ['drive', 'workshop', 'garage', 'session', 'connect'];
      for (var j = 0; j < pages.length; j++) id('page-' + pages[j]).hidden = pages[j] !== page;
    }
  }, false);

  id('pairButton').addEventListener('click', pair, false);
  id('resumeButton').addEventListener('click', resume, false);
  id('cancelButton').addEventListener('click', function () {
    api('POST', '/api/unpair', {}, function () { snapshot = null; stateName = 'unpaired'; show('pair'); updateResume(); });
  }, false);
  id('catalogButton').addEventListener('click', function () {
    if (selectedModel) command('catalog', {});
    else command('catalog', {});
    if (selectedModel) { selectedModel = ''; selectedConfig = ''; garageKey = ''; }
  }, false);
  id('backModels').addEventListener('click', function () {
    selectedModel = ''; selectedConfig = ''; garageKey = ''; command('catalog', {});
  }, false);
  id('spawnButton').addEventListener('click', function () {
    if (selectedConfig && spawnable()) command('spawn', { config: selectedConfig });
  }, false);
  id('replaceButton').addEventListener('click', function () {
    if (selectedConfig && mutable()) command('replace', { config: selectedConfig });
  }, false);
  id('disconnectButton').addEventListener('click', function () {
    api('POST', '/api/unpair', {}, function () {
      snapshot = null; stateName = 'unpaired'; show('pair'); updateResume();
      pairMessage('Browser disconnected. Pair again to resume control.');
    });
  }, false);
  id('forgetButton').addEventListener('click', function () {
    clearTrusted(); updateResume();
    message('Saved browser credential removed from this browser. Revoke it on the PC too if needed.');
  }, false);

  updateResume();
  poll();
})();
