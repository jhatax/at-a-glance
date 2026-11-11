const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { createContext, runInContext } = require('node:vm');

const pkjsPath = join(__dirname, '../../src/pkjs');
const manifest = require('../../package.json').pebble;
const messageKeys = Object.fromEntries(manifest.messageKeys.map((key, i) => [key, 10000 + i]));
const originalLocation = { latitude: 37, longitude: -122 };
const movedLocation = { latitude: 37.05, longitude: -122 };
const weatherResponse = {
  current: { temperature_2m: 20, weather_code: 0, is_day: 1 },
};

// Let the production promise chain finish without advancing the mocked timers.
function settlePromises() {
  return new Promise(setImmediate);
}

function createHarness({ savedSettings = null } = {}) {
  const listeners = new Map();
  const storage = new Map();
  if (savedSettings !== null) {
    storage.set('clay-settings', savedSettings);
  }
  const positions = [];
  const requests = [];
  const messages = [];
  const timers = new Map();
  let nextTimerId = 1;

  class MockXMLHttpRequest {
    open(method, url) {
      this.url = url;
      this.kind = new URL(url).hostname === 'nominatim.openstreetmap.org' ? 'lookup' : 'weather';
      this.completed = false;
    }

    setRequestHeader() {}

    send() {
      requests.push(this);
    }
  }

  const context = createContext({
    localStorage: {
      getItem(key) {
        return storage.has(key) ? storage.get(key) : null;
      },
      setItem(key, value) {
        storage.set(key, String(value));
      },
    },
    Pebble: {
      addEventListener(name, handler) {
        assert.equal(listeners.has(name), false, `Duplicate ${name} listener`);
        listeners.set(name, handler);
      },
      sendAppMessage(payload, success) {
        messages.push({ ...payload });
        if (success) {
          success();
        }
      },
    },
    console: { log() {} },
    navigator: {
      geolocation: {
        getCurrentPosition(success, failure) {
          positions.push({ success, failure });
        },
      },
    },
    XMLHttpRequest: MockXMLHttpRequest,
    setTimeout(callback, delay) {
      const id = nextTimerId++;
      timers.set(id, { callback, delay });
      return id;
    },
    clearTimeout(id) {
      timers.delete(id);
    },
  });

  // Run production modules in one PKJS environment, with CommonJS module scope.
  const modules = new Map();
  function load(name) {
    if (name === '@rebble/clay') {
      return function Clay() {};
    }
    if (name === 'message_keys') {
      return messageKeys;
    }
    if (name === './config.json') {
      return require('../../src/pkjs/config.json');
    }
    assert.ok(
      name === './app_message_queue' || name === './weather_location',
      `Unexpected dependency: ${name}`
    );
    if (!modules.has(name)) {
      const filename = join(pkjsPath, name + '.js');
      const module = { exports: {} };
      modules.set(name, module);
      const evaluate = runInContext(
        '(function (module, exports, require) {\n' + readFileSync(filename, 'utf8') + '\n})',
        context,
        { filename }
      );
      evaluate(module, module.exports, load);
    }
    return modules.get(name).exports;
  }

  context.require = load;
  const filename = join(pkjsPath, 'index.js');
  runInContext(readFileSync(filename, 'utf8'), context, { filename });

  function position(coords = originalLocation) {
    const request = positions.shift();
    assert.ok(request, 'Expected a pending GPS request');
    request.success({ coords });
  }

  function request(kind) {
    // An older lookup may still be pending in the late-response tests.
    const pending = requests.filter((item) => item.kind === kind && !item.completed);
    assert.ok(pending.length, `Expected a pending ${kind} request`);
    return pending[pending.length - 1];
  }

  function respond(target, body, status = 200) {
    assert.equal(target.completed, false, 'Request already completed');
    target.completed = true;
    target.status = status;
    target.responseText = typeof body === 'string' ? body : JSON.stringify(body);
    target.onload();
  }

  function fail(kind, event = 'onerror') {
    const target = request(kind);
    target.completed = true;
    target.status = 0;
    target[event]();
  }

  async function tick() {
    assert.equal(timers.size, 1, 'Expected exactly one scheduled refresh');
    const [id, timer] = timers.entries().next().value;
    timers.delete(id);
    timer.callback();
    await settlePromises();
  }

  async function completeRefresh(coords = originalLocation, name = 'City A') {
    position(coords);
    await settlePromises();
    respond(request('weather'), weatherResponse);
    if (name !== null) {
      respond(request('lookup'), { address: { city: name } });
    }
    await settlePromises();
  }

  function requestCount(kind) {
    return requests.filter((item) => item.kind === kind).length;
  }

  return {
    // Direct controller calls share the same instance used by Pebble events.
    start: (minutes) => context.startRefresh(minutes),
    emit(name, event) {
      assert.ok(listeners.has(name), `Missing ${name} listener`);
      listeners.get(name)(event);
    },
    positions,
    requests,
    messages,
    timers,
    position,
    request,
    respond,
    fail,
    tick,
    completeRefresh,
    requestCount,
  };
}

module.exports = {
  createHarness,
  settlePromises,
  originalLocation,
  movedLocation,
  weatherResponse,
};
