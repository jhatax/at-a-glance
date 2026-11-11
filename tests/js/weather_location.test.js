const assert = require('node:assert/strict');
const { test } = require('node:test');
const {
  createHarness,
  settlePromises,
  originalLocation,
  movedLocation,
  weatherResponse,
} = require('./harness');

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

test('resync resends the cached pair without replacing the timer or fetching', async () => {
  const harness = createHarness();
  harness.start(30);
  await settlePromises();
  await harness.completeRefresh();
  const timer = [...harness.timers.entries()][0];
  const payload = harness.messages[0];
  const requestCount = harness.requests.length;

  harness.start(30);
  await settlePromises();

  assert.deepEqual(harness.messages, [payload, payload]);
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, requestCount);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.get(timer[0]), timer[1]);
});

for (const failure of ['weather', 'lookup']) {
  test(`resync after ${failure} failure cannot resend the previous pair`, async () => {
    const harness = createHarness();
    harness.start(15);
    await settlePromises();
    await harness.completeRefresh();
    await harness.tick();
    harness.position(movedLocation);
    await settlePromises();
    const pending = harness.request(failure === 'weather' ? 'lookup' : 'weather');
    harness.fail(failure);
    await settlePromises();
    const timer = [...harness.timers.entries()][0];

    // The other request may still complete after Promise.all has rejected.
    harness.respond(
      pending,
      failure === 'weather' ? { address: { city: 'City B' } } : weatherResponse
    );
    await settlePromises();
    harness.start(15);
    await settlePromises();

    assert.deepEqual(harness.messages, [
      { TEMPERATURE: 200, WEATHER_CONDITION: 0, IS_DAY: 1, MAYBE_CURRENT_LOCATION: 'CITY A' },
      { TEMPERATURE: 0, WEATHER_CONDITION: -1, IS_DAY: 0, MAYBE_CURRENT_LOCATION: '' },
    ]);
    assert.equal(harness.positions.length, 0);
    assert.equal(harness.requests.length, 4);
    assert.equal(harness.timers.size, 1);
    assert.equal(harness.timers.get(timer[0]), timer[1]);
  });
}

test('a late climate response cannot overwrite the newer cached pair on resync', async () => {
  const harness = createHarness();
  harness.start(15);
  await settlePromises();
  harness.position();
  await settlePromises();
  const oldWeather = harness.request('weather');

  // Resync queues another attempt while the first attempt is still running.
  harness.start(15);
  harness.fail('lookup');
  await settlePromises();
  await harness.completeRefresh(movedLocation, 'City B');
  harness.respond(oldWeather, {
    current: { temperature_2m: 5, weather_code: 3, is_day: 0 },
  });
  await settlePromises();
  assert.equal(harness.messages.length, 2);

  harness.start(15);
  await settlePromises();

  const expected = {
    TEMPERATURE: 200,
    WEATHER_CONDITION: 0,
    IS_DAY: 1,
    MAYBE_CURRENT_LOCATION: 'CITY B',
  };
  assert.deepEqual(harness.messages.slice(1), [expected, expected]);
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 4);
  assert.equal(harness.timers.size, 1);
});

for (const [minutes, expected] of [
  [60, 60],
  [61, 15],
  [0, 15],
  [-15, 15],
  [NaN, 15],
  [Infinity, 15],
]) {
  test(`interval ${minutes} schedules refreshes every ${expected} minutes`, async () => {
    const harness = createHarness();
    harness.start(minutes);
    await settlePromises();
    await harness.completeRefresh();

    assert.equal(harness.messages.length, 1);
    assert.equal(harness.timers.size, 1);
    assert.equal(harness.timers.values().next().value.delay, expected * 60 * 1000);
  });
}
