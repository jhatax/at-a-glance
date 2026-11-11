const assert = require('node:assert/strict');
const { test } = require('node:test');
const { createHarness, settlePromises } = require('./harness');

test('ready waits for the sentinel acknowledgment before starting the saved interval', async () => {
  const harness = createHarness({
    savedSettings: JSON.stringify({ WEATHER_UPDATE_MINUTES: '45' }),
  });
  harness.emit('ready');
  await settlePromises();

  assert.deepEqual(harness.messages, [{ SENTINEL: 1 }]);
  // Successful transport delivery alone must not start climate/location polling.
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 0);
  assert.equal(harness.timers.size, 0);

  harness.emit('appmessage', { payload: { SENTINEL: 1 } });
  await settlePromises();
  assert.equal(harness.positions.length, 1);
  await harness.completeRefresh();

  assert.deepEqual(harness.messages, [
    { SENTINEL: 1 },
    { TEMPERATURE: 200, WEATHER_CONDITION: 0, IS_DAY: 1, MAYBE_CURRENT_LOCATION: 'CITY A' },
  ]);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.values().next().value.delay, 45 * 60 * 1000);
});

for (const [label, settings] of [
  ['missing settings', null],
  ['missing interval', '{}'],
  ['unparseable interval', '{"WEATHER_UPDATE_MINUTES":"invalid"}'],
]) {
  test(`sentinel acknowledgment uses the default for ${label}`, async () => {
    const harness = createHarness({ savedSettings: settings });
    harness.emit('ready');
    await settlePromises();
    assert.equal(harness.positions.length, 0);
    assert.equal(harness.requests.length, 0);

    harness.emit('appmessage', { payload: { SENTINEL: 1 } });
    await settlePromises();
    await harness.completeRefresh();

    assert.equal(harness.messages.length, 2);
    assert.equal(harness.timers.size, 1);
    assert.equal(harness.timers.values().next().value.delay, 15 * 60 * 1000);
  });
}

test('unrelated or incomplete messages do not start climate/location polling', async () => {
  const harness = createHarness();
  harness.emit('ready');
  for (const event of [
    undefined,
    {},
    { payload: null },
    { payload: {} },
    { payload: { WEATHER_UPDATE_MINUTES: 30 } },
    { payload: { SENTINEL: 0 } },
    { payload: { SENTINEL: 2 } },
    { payload: { SENTINEL: '1' } },
  ]) {
    harness.emit('appmessage', event);
  }
  await settlePromises();

  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 0);
  assert.equal(harness.timers.size, 0);
  assert.deepEqual(harness.messages, [{ SENTINEL: 1 }]);
});

test('a repeated sentinel acknowledgment resends the cached pair through the climate queue', async () => {
  const harness = createHarness({
    savedSettings: JSON.stringify({ WEATHER_UPDATE_MINUTES: 30 }),
  });
  harness.emit('ready');
  harness.emit('appmessage', { payload: { SENTINEL: 1 } });
  await settlePromises();
  await harness.completeRefresh();
  const timer = [...harness.timers.entries()][0];

  harness.emit('appmessage', { payload: { SENTINEL: 1 } });
  await settlePromises();

  const expected = {
    TEMPERATURE: 200,
    WEATHER_CONDITION: 0,
    IS_DAY: 1,
    MAYBE_CURRENT_LOCATION: 'CITY A',
  };
  assert.deepEqual(harness.messages, [{ SENTINEL: 1 }, expected, expected]);
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 2);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.get(timer[0]), timer[1]);
});
