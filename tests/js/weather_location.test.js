const assert = require('node:assert/strict');
const { test } = require('node:test');
const {
  createHarness,
  settlePromises,
  originalLocation,
  movedLocation,
  weatherResponse,
} = require('./harness');

test('REFRESH fetches a combined payload without scheduling another request', async () => {
  const harness = createHarness();
  await harness.refresh();

  assert.equal(harness.positions.length, 1);
  assert.equal(harness.timers.size, 0);
  await harness.completeRefresh();

  assert.deepEqual(harness.messages, [
    { TEMPERATURE: 200, WEATHER_CONDITION: 0, IS_DAY: 1, MAYBE_CURRENT_LOCATION: 'CITY A' },
  ]);
  assert.equal(harness.timers.size, 0);
});

test('four REFRESH requests produce four updates without a JS cadence timer', async () => {
  const harness = createHarness();
  await harness.refresh();
  await harness.completeRefresh();

  for (let cycle = 0; cycle < 3; cycle++) {
    await harness.refresh();
    await harness.completeRefresh(originalLocation, null);
  }

  assert.equal(harness.requestCount('weather'), 4);
  assert.equal(harness.requestCount('lookup'), 1);
  assert.equal(harness.messages.length, 4);
  assert.equal(harness.timers.size, 0);
});

test('movement within the tolerance reuses the name and coordinate anchor', async () => {
  const harness = createHarness();
  await harness.refresh();
  await harness.completeRefresh();

  await harness.refresh();
  await harness.completeRefresh({ latitude: 37.015, longitude: -122.015 }, null);

  const url = new URL(harness.requests.filter((item) => item.kind === 'weather').at(-1).url);
  assert.equal(url.searchParams.get('latitude'), '37');
  assert.equal(url.searchParams.get('longitude'), '-122');
  assert.equal(harness.requestCount('lookup'), 1);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY A');
});

test('movement beyond the tolerance fetches a new location name', async () => {
  const harness = createHarness();
  await harness.refresh();
  await harness.completeRefresh();

  await harness.refresh();
  await harness.completeRefresh({ latitude: 37.021, longitude: -122 }, 'City B');

  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY B');
});

test('GPS failure clears the display and the next refresh recovers', async () => {
  const harness = createHarness();
  await harness.refresh();
  harness.positions.shift().failure({ code: 1, message: 'GPS unavailable' });
  await settlePromises();

  assert.equal(harness.messages.at(-1).WEATHER_CONDITION, -1);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, '');
  await harness.refresh();
  await harness.completeRefresh();
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY A');
});

test('weather failure clears both caches and forces a new lookup', async () => {
  const harness = createHarness();
  await harness.refresh();
  await harness.completeRefresh();

  await harness.refresh();
  harness.position();
  await settlePromises();
  harness.fail('weather');
  await settlePromises();
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, '');

  await harness.refresh();
  await harness.completeRefresh(originalLocation, 'New Name');
  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'NEW NAME');
});

test('a late lookup cannot refill the cache after failure clears the coordinates', async () => {
  const harness = createHarness();
  await harness.refresh();
  harness.position();
  await settlePromises();
  const oldLookup = harness.request('lookup');

  harness.fail('weather');
  await settlePromises();
  harness.respond(oldLookup, { address: { city: 'Old Name' } });
  await settlePromises();
  assert.equal(harness.messages.length, 1);

  await harness.refresh();
  await harness.completeRefresh(originalLocation, 'Fresh Name');
  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'FRESH NAME');
});

test('a late old lookup cannot overwrite the newer location cache', async () => {
  const harness = createHarness();
  await harness.refresh();
  harness.position();
  await settlePromises();
  const oldLookup = harness.request('lookup');

  await harness.refresh();
  harness.fail('weather');
  await settlePromises();
  await harness.completeRefresh(movedLocation, 'City B');
  harness.respond(oldLookup, { address: { city: 'City A' } });
  await settlePromises();

  await harness.refresh();
  await harness.completeRefresh(movedLocation, null);
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY B');
  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.timers.size, 0);
});

test('REFRESH requests during a refresh preserve ordering without JS timers', async () => {
  const harness = createHarness();
  await harness.refresh();
  await harness.refresh();
  await harness.refresh();
  assert.equal(harness.positions.length, 1);

  await harness.completeRefresh();
  assert.equal(harness.positions.length, 1);
  await harness.completeRefresh(originalLocation, null);
  assert.equal(harness.positions.length, 1);
  await harness.completeRefresh(originalLocation, null);
  assert.equal(harness.timers.size, 0);

  await harness.refresh();
  await harness.completeRefresh(originalLocation, null);
  assert.equal(harness.timers.size, 0);
});

test('lookup timeout clears the display and the next REFRESH recovers', async () => {
  const harness = createHarness();
  await harness.refresh();
  harness.position();
  await settlePromises();
  harness.respond(harness.request('weather'), weatherResponse);
  harness.fail('lookup', 'ontimeout');
  await settlePromises();
  assert.equal(harness.messages.at(-1).WEATHER_CONDITION, -1);

  await harness.refresh();
  await harness.completeRefresh();
  assert.equal(harness.messages.at(-1).MAYBE_CURRENT_LOCATION, 'CITY A');
});

test('malformed weather JSON clears the display without scheduling another request', async () => {
  const harness = createHarness();
  await harness.refresh();
  harness.position();
  await settlePromises();
  harness.respond(harness.request('lookup'), { address: { city: 'City A' } });
  harness.respond(harness.request('weather'), '{bad');
  await settlePromises();

  assert.equal(harness.messages.at(-1).WEATHER_CONDITION, -1);
  assert.equal(harness.timers.size, 0);
});

test('HTTP lookup failure clears the display without scheduling another request', async () => {
  const harness = createHarness();
  await harness.refresh();
  harness.position();
  await settlePromises();
  harness.respond(harness.request('weather'), weatherResponse);
  harness.respond(harness.request('lookup'), {}, 500);
  await settlePromises();

  assert.equal(harness.messages.at(-1).WEATHER_CONDITION, -1);
  assert.equal(harness.timers.size, 0);
});

test('a new REFRESH waits for fresh climate instead of immediately resending the cached pair', async () => {
  const harness = createHarness();
  await harness.refresh();
  await harness.completeRefresh();

  await harness.refresh();
  assert.equal(harness.positions.length, 1);
  assert.equal(harness.messages.length, 1);
  harness.position();
  await settlePromises();
  assert.equal(harness.requestCount('lookup'), 1);
  assert.equal(harness.messages.length, 1);
  harness.respond(harness.request('weather'), {
    current: { temperature_2m: 25, weather_code: 3, is_day: 0 },
  });
  await settlePromises();

  assert.deepEqual(harness.messages.at(-1), {
    TEMPERATURE: 250,
    WEATHER_CONDITION: 3,
    IS_DAY: 0,
    MAYBE_CURRENT_LOCATION: 'CITY A',
  });
  assert.equal(harness.messages.length, 2);
  assert.equal(harness.timers.size, 0);
});

for (const failure of ['weather', 'lookup']) {
  test(`REFRESH after ${failure} failure cannot resend the previous pair`, async () => {
    const harness = createHarness();
    await harness.refresh();
    await harness.completeRefresh();
    await harness.refresh();
    harness.position(movedLocation);
    await settlePromises();
    const pending = harness.request(failure === 'weather' ? 'lookup' : 'weather');
    harness.fail(failure);
    await settlePromises();

    // The other request may still complete after Promise.all has rejected.
    harness.respond(
      pending,
      failure === 'weather' ? { address: { city: 'City B' } } : weatherResponse
    );
    await settlePromises();
    await harness.refresh();

    assert.deepEqual(harness.messages, [
      { TEMPERATURE: 200, WEATHER_CONDITION: 0, IS_DAY: 1, MAYBE_CURRENT_LOCATION: 'CITY A' },
      { TEMPERATURE: 0, WEATHER_CONDITION: -1, IS_DAY: 0, MAYBE_CURRENT_LOCATION: '' },
    ]);
    assert.equal(harness.positions.length, 1);
    await harness.completeRefresh(movedLocation, 'City B');
    assert.deepEqual(harness.messages.at(-1), {
      TEMPERATURE: 200,
      WEATHER_CONDITION: 0,
      IS_DAY: 1,
      MAYBE_CURRENT_LOCATION: 'CITY B',
    });
    assert.equal(harness.messages.length, 3);
    assert.equal(harness.requestCount('lookup'), 3);
    assert.equal(harness.timers.size, 0);
  });
}

test('a late climate response from a failed attempt cannot overwrite a newer successful update', async () => {
  const harness = createHarness();
  await harness.refresh();
  harness.position();
  await settlePromises();
  const oldWeather = harness.request('weather');

  // Another watch request queues an attempt while the first is still running.
  await harness.refresh();
  harness.fail('lookup');
  await settlePromises();
  await harness.completeRefresh(movedLocation, 'City B');
  harness.respond(oldWeather, {
    current: { temperature_2m: 5, weather_code: 3, is_day: 0 },
  });
  await settlePromises();
  assert.equal(harness.messages.length, 2);

  await harness.refresh();
  await harness.completeRefresh(movedLocation, null);

  const expected = {
    TEMPERATURE: 200,
    WEATHER_CONDITION: 0,
    IS_DAY: 1,
    MAYBE_CURRENT_LOCATION: 'CITY B',
  };
  assert.deepEqual(harness.messages.slice(1), [expected, expected]);
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requestCount('weather'), 3);
  assert.equal(harness.requestCount('lookup'), 2);
  assert.equal(harness.timers.size, 0);
});
