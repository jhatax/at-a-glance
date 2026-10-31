const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { test } = require('node:test');
const { runInNewContext } = require('node:vm');

const sourcePath = join(__dirname, '../../src/pkjs/weather_location.js');
const source = readFileSync(sourcePath, 'utf8');
const originalLocation = { latitude: 37, longitude: -122 };
const movedLocation = { latitude: 37.05, longitude: -122 };
const weatherResponse = {
  current: { temperature_2m: 20, weather_code: 0, is_day: 1 },
};

// Let the production promise chain finish without advancing the mocked timers.
function settlePromises() {
  return new Promise(setImmediate);
}

function createHarness() {
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

  const context = {
    module: { exports: {} },
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
  };

  runInNewContext(source, context, { filename: sourcePath });
  const start = context.module.exports.createRefreshController((payload) => {
    messages.push({ ...payload });
  });

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
    start,
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

test('startup fetches weather and location before scheduling the next refresh', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();

  assert.equal(harness.positions.length, 1);
  assert.equal(harness.timers.size, 0);
  await harness.completeRefresh();

  assert.deepEqual(harness.messages, [
    { TEMPERATURE: 200, WEATHER_CONDITION: 0, IS_DAY: 1, MAYBE_CURRENT_LOCATION: 'CITY A' },
  ]);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.values().next().value.delay, 15 * 60 * 1000);
});

test('polling continues across four refreshes with one timer', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  await harness.completeRefresh();

  for (let cycle = 0; cycle < 3; cycle++) {
    await harness.tick();
    await harness.completeRefresh(originalLocation, null);
  }

  assert.equal(harness.requestCount('weather'), 4);
  assert.equal(harness.requestCount('lookup'), 1);
  assert.equal(harness.messages.length, 4);
  assert.equal(harness.timers.size, 1);
});

test('movement within the tolerance reuses the name and coordinate anchor', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  await harness.completeRefresh();

  await harness.tick();
  await harness.completeRefresh({ latitude: 37.015, longitude: -122.015 }, null);

  const url = new URL(harness.requests.filter((item) => item.kind === 'weather').at(-1).url);
  assert.equal(url.searchParams.get('latitude'), '37');
  assert.equal(url.searchParams.get('longitude'), '-122');
  assert.equal(harness.requestCount('lookup'), 1);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY A');
});

test('movement beyond the tolerance fetches a new location name', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  await harness.completeRefresh();

  await harness.tick();
  await harness.completeRefresh({ latitude: 37.021, longitude: -122 }, 'City B');

  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY B');
});

test('GPS failure clears the display and the next refresh recovers', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  harness.positions.shift().failure({ code: 1, message: 'GPS unavailable' });
  await settlePromises();

  assert.equal(harness.messages.at(-1).WEATHER_CONDITION, -1);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, '');
  await harness.tick();
  await harness.completeRefresh();
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY A');
});

test('weather failure clears both caches and forces a new lookup', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  await harness.completeRefresh();

  await harness.tick();
  harness.position();
  await settlePromises();
  harness.fail('weather');
  await settlePromises();
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, '');

  await harness.tick();
  await harness.completeRefresh(originalLocation, 'New Name');
  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'NEW NAME');
});

test('a late lookup cannot refill the cache after failure clears the coordinates', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  harness.position();
  await settlePromises();
  const oldLookup = harness.request('lookup');

  harness.fail('weather');
  await settlePromises();
  harness.respond(oldLookup, { address: { city: 'Old Name' } });
  await settlePromises();
  assert.equal(harness.messages.length, 1);

  await harness.tick();
  await harness.completeRefresh(originalLocation, 'Fresh Name');
  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'FRESH NAME');
});

test('a late old lookup cannot overwrite the newer location cache', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  harness.position();
  await settlePromises();
  const oldLookup = harness.request('lookup');

  harness.start(30);
  harness.fail('weather');
  await settlePromises();
  await harness.completeRefresh(movedLocation, 'City B');
  harness.respond(oldLookup, { address: { city: 'City A' } });
  await settlePromises();

  harness.start(45);
  await settlePromises();
  await harness.completeRefresh(movedLocation, null);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY B');
  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.timers.size, 1);
});

test('interval changes during a refresh preserve ordering and leave one timer', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  harness.start(30);
  harness.start(45);
  await settlePromises();
  assert.equal(harness.positions.length, 1);

  await harness.completeRefresh();
  assert.equal(harness.positions.length, 1);
  await harness.completeRefresh(originalLocation, null);
  assert.equal(harness.positions.length, 1);
  await harness.completeRefresh(originalLocation, null);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.values().next().value.delay, 45 * 60 * 1000);

  await harness.tick();
  await harness.completeRefresh(originalLocation, null);
  assert.equal(harness.timers.size, 1);
});

test('lookup timeout clears the display and polling recovers', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  harness.position();
  await settlePromises();
  harness.respond(harness.request('weather'), weatherResponse);
  harness.fail('lookup', 'ontimeout');
  await settlePromises();
  assert.equal(harness.messages.at(-1).WEATHER_CONDITION, -1);

  await harness.tick();
  await harness.completeRefresh();
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY A');
});

test('malformed weather JSON clears the display and schedules another refresh', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  harness.position();
  await settlePromises();
  harness.respond(harness.request('lookup'), { address: { city: 'City A' } });
  harness.respond(harness.request('weather'), '{bad');
  await settlePromises();

  assert.equal(harness.messages.at(-1).WEATHER_CONDITION, -1);
  assert.equal(harness.timers.size, 1);
});

test('HTTP lookup failure clears the display and schedules another refresh', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  harness.position();
  await settlePromises();
  harness.respond(harness.request('weather'), weatherResponse);
  harness.respond(harness.request('lookup'), {}, 500);
  await settlePromises();

  assert.equal(harness.messages.at(-1).WEATHER_CONDITION, -1);
  assert.equal(harness.timers.size, 1);
});
