const assert = require('node:assert/strict');
const { test } = require('node:test');
const { createHarness, settlePromises } = require('./harness');

test('ready announces JS readiness and waits for a REFRESH request', async () => {
  const harness = createHarness({
    savedSettings: JSON.stringify({ WEATHER_UPDATE_MINUTES: '45' }),
  });
  harness.emit('ready');
  await settlePromises();

  assert.deepEqual(harness.messages, [{ JS_READY: 1 }]);
  // Successful transport delivery alone must not request climate/location.
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 0);
  assert.equal(harness.timers.size, 0);

  await harness.refresh();
  assert.equal(harness.positions.length, 1);
  await harness.completeRefresh();

  assert.deepEqual(harness.messages, [
    { JS_READY: 1 },
    { TEMPERATURE: 200, WEATHER_CONDITION: 0, IS_DAY: 1, MAYBE_CURRENT_LOCATION: 'CITY A' },
  ]);
  assert.equal(harness.timers.size, 0);
});

for (const [label, settings] of [
  ['missing settings', null],
  ['missing interval', '{}'],
  ['unparseable interval', '{"WEATHER_UPDATE_MINUTES":"invalid"}'],
]) {
  test(`REFRESH works without JS scheduling for ${label}`, async () => {
    const harness = createHarness({ savedSettings: settings });
    harness.emit('ready');
    await settlePromises();
    assert.equal(harness.positions.length, 0);
    assert.equal(harness.requests.length, 0);

    await harness.refresh();
    await harness.completeRefresh();

    assert.equal(harness.messages.length, 2);
    assert.equal(harness.timers.size, 0);
  });
}

test('unrelated or incomplete messages do not request climate/location', async () => {
  const harness = createHarness();
  harness.emit('ready');
  for (const event of [
    undefined,
    {},
    { payload: null },
    { payload: {} },
    { payload: { WEATHER_UPDATE_MINUTES: 30 } },
    { payload: { JS_READY: 1 } },
    { payload: { REFRESH: 0 } },
    { payload: { REFRESH: 2 } },
    { payload: { REFRESH: '1' } },
  ]) {
    harness.emit('appmessage', event);
  }
  await settlePromises();

  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 0);
  assert.equal(harness.timers.size, 0);
  assert.deepEqual(harness.messages, [{ JS_READY: 1 }]);
});

test('each REFRESH fetches climate again and reuses the location lookup when stationary', async () => {
  const harness = createHarness({
    savedSettings: JSON.stringify({ WEATHER_UPDATE_MINUTES: 30 }),
  });
  harness.emit('ready');
  await harness.refresh();
  await harness.completeRefresh();

  await harness.refresh();
  assert.equal(harness.positions.length, 1);
  assert.equal(harness.messages.length, 2);
  await harness.completeRefresh(undefined, null);

  const expected = {
    TEMPERATURE: 200,
    WEATHER_CONDITION: 0,
    IS_DAY: 1,
    MAYBE_CURRENT_LOCATION: 'CITY A',
  };
  assert.deepEqual(harness.messages, [{ JS_READY: 1 }, expected, expected]);
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requestCount('weather'), 2);
  assert.equal(harness.requestCount('lookup'), 1);
  assert.equal(harness.timers.size, 0);
});
