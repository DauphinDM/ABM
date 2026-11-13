const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

// Exercise the shipped ES5 client without a server, game, or browser dependency.
function client(saved) {
    const nodes = new Map(), all = [], requests = [], timers = new Map(), storage = new Map();
    let timerId = 0;
    class Element {
        constructor(tag = 'div') {
            this.tagName = tag; this.children = []; this.attributes = {}; this.listeners = {};
            this.textContent = ''; this.value = ''; this.disabled = false; this.hidden = false;
            all.push(this);
        }
        get firstChild() { return this.children[0]; }
        get lastChild() { return this.children[this.children.length - 1]; }
        appendChild(child) { child.parentNode = this; this.children.push(child); return child; }
        removeChild(child) { this.children.splice(this.children.indexOf(child), 1); }
        insertBefore(child, next) {
            child.parentNode = this; this.children.splice(this.children.indexOf(next), 0, child);
        }
        setAttribute(key, value) { this.attributes[key] = String(value); }
        getAttribute(key) { return this.attributes[key] === undefined ? null : this.attributes[key]; }
        addEventListener(type, handler) { this.listeners[type] = handler; }
    }
    const document = {
        listeners: {},
        getElementById(name) { return nodes.get(name) || all.find(node => node.id === name); },
        createElement(tag) { return new Element(tag); },
        querySelectorAll(selector) { return all.filter(node => node.getAttribute(selector.slice(1, -1)) !== null); },
        addEventListener(type, handler) { this.listeners[type] = handler; }
    };
    const html = fs.readFileSync('src/web/index.html', 'utf8');
    for (const match of html.matchAll(/<(\w+)\b([^>]*)(?:>|$)/g)) {
        const node = new Element(match[1]);
        for (const attr of match[2].matchAll(/([\w-]+)="([^"]*)"/g)) {
            node.setAttribute(attr[1], attr[2]);
            if (attr[1] === 'id') { node.id = attr[2]; nodes.set(node.id, node); }
            if (attr[1] === 'value') node.value = attr[2];
        }
    }
    const playerParent = new Element(); playerParent.appendChild(nodes.get('players'));
    if (saved) storage.set('abm_web_trusted', JSON.stringify(saved));
    class XMLHttpRequest {
        open(method, path) { this.method = method; this.path = path; this.headers = {}; }
        setRequestHeader(key, value) { this.headers[key] = value; }
        send(body) { this.body = body === null ? null : JSON.parse(body); requests.push(this); }
        reply(status, data) {
            this.status = status; this.responseText = JSON.stringify(data); this.readyState = 4;
            this.onreadystatechange();
        }
        expire() {
            this.reply(0, null);
            this.ontimeout(); // XHR can notify both readyState and timeout.
        }
    }
    const context = vm.createContext({
        document, XMLHttpRequest,
        localStorage: { getItem: key => storage.get(key) || null, setItem: (key, value) => storage.set(key, value), removeItem: key => storage.delete(key) },
        atob: value => Buffer.from(value, 'base64').toString('binary'),
        setTimeout: fn => { timers.set(++timerId, fn); return timerId; },
        clearTimeout: key => timers.delete(key)
    });
    vm.runInContext(fs.readFileSync('src/web/app.js', 'utf8'), context);
    return {
        nodes, requests, timers, storage,
        click(name) {
            const node = nodes.get(name);
            if (node.disabled) return;
            if (node.listeners.click) node.listeners.click();
            document.listeners.click({ target: node });
        },
        poll() {
            assert.equal(timers.size, 1);
            const [key, fn] = timers.entries().next().value; timers.delete(key); fn();
            return requests[requests.length - 1];
        }
    };
}

function approved(generation = 'first') {
    return { status: 'approved', device: 'Test browser', seq: 0, snapshot: {
        state: { connected: true, controlsAvailable: true, vehicleOwned: true, sandboxAllowed: true,
            map: 'test', vehicleId: 42, telemetry: { speed: 0, rpm: 800 } },
        target: { session: 'test', vehicle: 42, generation }, catalog: [], configs: []
    } };
}

// Saved access never resumes without an explicit click.
const saved = { id: 'a'.repeat(32), token: 'b'.repeat(64) };
const c = client(saved);
assert.deepEqual(c.requests.map(r => r.path), ['/api/state']);
c.requests[0].reply(401, { status: 'unpaired' });
assert.equal(c.nodes.get('resumeButton').hidden, false);
c.click('resumeButton');
assert.equal(c.requests[1].path, '/api/resume');
assert.deepEqual(JSON.parse(JSON.stringify(c.requests[1].body)), saved);
c.requests[1].reply(200, { status: 'approved' });
c.requests[2].reply(200, approved());
assert.equal(c.nodes.get('powerApply').disabled, false);

// A stalled poll ends once, clears telemetry, locks controls, and keeps retrying.
const stalled = c.poll(); assert.equal(stalled.timeout, 5000); stalled.expire();
assert.equal(c.timers.size, 1);
assert.equal(c.nodes.get('powerApply').disabled, true);
assert.equal(c.nodes.get('catalogButton').disabled, true);
assert.equal(c.nodes.get('resetTrip').disabled, true);
assert.equal(c.nodes.get('speed').textContent, '--');
assert.equal(c.nodes.get('badge').textContent, 'RECONNECTING');
const count = c.requests.length; c.click('powerApply'); assert.equal(c.requests.length, count);

// Recovery uses a fresh target; timed-out mutations are never replayed.
c.poll().reply(200, approved('new-generation'));
assert.equal(c.nodes.get('powerApply').disabled, false);
c.nodes.get('power').value = '150'; c.click('powerApply');
const power = c.requests[c.requests.length - 1];
assert.equal(power.path, '/api/command');
assert.equal(power.body.target.generation, 'new-generation');
assert.equal(power.body.args.value, 1.5);
assert.equal(power.body.seq, 1);
power.expire();
c.poll().reply(200, approved('new-generation'));
assert.equal(c.requests.filter(r => r.path === '/api/command').length, 1);

// Empty maps allow spawning, but not tuning or replacement.
const empty = approved(); delete empty.snapshot.state.vehicleId; empty.snapshot.state.vehicleOwned = false;
c.poll().reply(200, empty);
assert.equal(c.nodes.get('powerApply').disabled, true);
assert.equal(c.nodes.get('replaceButton').disabled, true);

// A server error also invalidates the last usable snapshot.
c.poll().reply(200, approved());
c.poll().reply(503, { error: 'Unavailable' });
assert.equal(c.nodes.get('powerApply').disabled, true);
assert.equal(c.nodes.get('badge').textContent, 'RECONNECTING');
const reloaded = approved(); reloaded.seq = 17;
c.poll().reply(200, reloaded); c.click('powerApply');
assert.equal(c.requests[c.requests.length - 1].body.seq, 18);

// Pairing links are decoded locally; the temporary secret is never persisted.
const p = client(); p.requests[0].reply(401, { status: 'unpaired' });
p.nodes.get('link').value = 'invalid'; p.click('pairButton'); assert.equal(p.requests.length, 1);
const secret = 'c'.repeat(64);
p.nodes.get('link').value = 'abm://pair/' + Buffer.from(JSON.stringify({ v: 1, secret })).toString('base64url');
p.click('pairButton');
assert.equal(p.requests[1].body.secret, secret);
assert.equal(p.requests[1].headers['X-ABM-Web'], '1');
p.requests[1].reply(200, { status: 'pending' });
assert.equal(p.nodes.get('link').value, '');
assert.equal(p.nodes.get('pendingView').hidden, false);
assert.equal(p.storage.size, 0);
p.poll().reply(200, approved());
p.poll().reply(401, { status: 'expired' });
assert.equal(p.nodes.get('appView').hidden, true);
assert.equal(p.nodes.get('pairView').hidden, false);

// An initial network failure must not permanently wedge the polling flag.
const offline = client(); offline.requests[0].expire(); offline.poll().reply(200, approved());
assert.equal(offline.nodes.get('appView').hidden, false);

// A network outage isn't revocation. Keep the token until the PC rejects it.
const resume = client(saved); resume.requests[0].reply(401, { status: 'unpaired' });
resume.click('resumeButton'); resume.requests[1].expire();
assert.equal(resume.storage.has('abm_web_trusted'), true);
assert.equal(resume.nodes.get('resumeButton').disabled, false);
resume.poll().reply(401, { status: 'unpaired' });
assert.equal(resume.requests.filter(r => r.path === '/api/resume').length, 1);
resume.click('resumeButton'); resume.requests[resume.requests.length - 1].reply(403, { error: 'Revoked' });
assert.equal(resume.storage.has('abm_web_trusted'), false);
assert.equal(resume.nodes.get('resumeButton').hidden, true);
console.log('Browser pairing, polling, stale-control and command tests passed');
