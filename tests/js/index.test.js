const assert = require('node:assert/strict');
const { test } = require('node:test');
const { createHarness, settlePromises, originalLocation } = require('./harness');

test('ready waits for the SYNC acknowledgment before starting the saved interval', async () => {
  const harness = createHarness({
    savedSettings: JSON.stringify({ WEATHER_UPDATE_MINUTES: '45' }),
  });
  harness.emit('ready');
  await settlePromises();

  assert.deepEqual(harness.messages, [{ SYNC: 1 }]);
  // Successful transport delivery alone must not start climate/location polling.
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 0);
  assert.equal(harness.timers.size, 0);

  harness.emit('appmessage', { payload: { SYNC: 1 } });
  await settlePromises();
  assert.equal(harness.positions.length, 1);
  await harness.completeRefresh();

  assert.deepEqual(harness.messages, [
    { SYNC: 1 },
    { TEMPERATURE: 200, WEATHER_CONDITION: 0, IS_DAY: 1, MAYBE_CURRENT_LOCATION: 'CITY A' },
  ]);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.values().next().value.delay, 45 * 60 * 1000);
});

for (const [label, settings] of [
  ['missing settings', null],
  ['missing interval', '{}'],
  ['malformed settings', '{bad'],
  ['unparseable interval', '{"WEATHER_UPDATE_MINUTES":"invalid"}'],
]) {
  test(`SYNC acknowledgment uses the default for ${label}`, async () => {
    const harness = createHarness({ savedSettings: settings });
    harness.emit('ready');
    await settlePromises();
    assert.equal(harness.positions.length, 0);
    assert.equal(harness.requests.length, 0);

    harness.emit('appmessage', { payload: { SYNC: 1 } });
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
    { payload: { SYNC: 0 } },
    { payload: { SYNC: 2 } },
    { payload: { SYNC: '1' } },
  ]) {
    harness.emit('appmessage', event);
  }
  await settlePromises();

  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 0);
  assert.equal(harness.timers.size, 0);
  assert.deepEqual(harness.messages, [{ SYNC: 1 }]);
});

test('a repeated SYNC acknowledgment resends the cached pair through the climate queue', async () => {
  const harness = createHarness({
    savedSettings: JSON.stringify({ WEATHER_UPDATE_MINUTES: 30 }),
  });
  harness.emit('ready');
  harness.emit('appmessage', { payload: { SYNC: 1 } });
  await settlePromises();
  await harness.completeRefresh();
  const timer = [...harness.timers.entries()][0];

  harness.emit('appmessage', { payload: { SYNC: 1 } });
  await settlePromises();

  const expected = {
    TEMPERATURE: 200,
    WEATHER_CONDITION: 0,
    IS_DAY: 1,
    MAYBE_CURRENT_LOCATION: 'CITY A',
  };
  assert.deepEqual(harness.messages, [{ SYNC: 1 }, expected, expected]);
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 2);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.get(timer[0]), timer[1]);
});

for (const [input, minutes] of [
  [0, 15],
  [-1, 15],
  [360, 60],
  ['60', 60],
]) {
  test(`saved interval ${input} uses ${minutes} minutes`, async () => {
    const harness = createHarness({
      savedSettings: JSON.stringify({ WEATHER_UPDATE_MINUTES: input }),
    });
    await harness.sync();
    await harness.completeRefresh();
    assert.equal(harness.timers.size, 1);
    assert.equal(harness.timers.values().next().value.delay, minutes * 60 * 1000);
  });
}

test('configuration changes replace the old timer and retain the interval on repeated SYNC', async () => {
  const harness = createHarness({
    savedSettings: JSON.stringify({ WEATHER_UPDATE_MINUTES: 45 }),
  });
  await harness.sync();
  await harness.completeRefresh();
  const oldTimerId = harness.timers.keys().next().value;

  harness.emit('webviewclosed', {
    response: JSON.stringify({ WEATHER_UPDATE_MINUTES: '30' }),
  });
  await settlePromises();
  assert.equal(harness.timers.has(oldTimerId), false);
  assert.equal(harness.timers.size, 0);
  assert.equal(harness.positions.length, 1);
  assert.equal(harness.messages.at(-1)[harness.messageKeys.WEATHER_UPDATE_MINUTES], 30);
  await harness.completeRefresh(originalLocation, null);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.values().next().value.delay, 30 * 60 * 1000);

  // SYNC reloads Clay storage; it must retain the configured interval and timer.
  const timer = [...harness.timers.entries()][0];
  await harness.sync();
  assert.equal(harness.positions.length, 0);
  assert.equal(harness.timers.get(timer[0]), timer[1]);
  await harness.advanceRefresh(30);
  await harness.completeRefresh(originalLocation, null);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.values().next().value.delay, 30 * 60 * 1000);
});

test('saving the same interval resends cached data without replacing the timer', async () => {
  const harness = createHarness();
  await harness.sync();
  await harness.completeRefresh();
  const timer = [...harness.timers.entries()][0];
  const cached = harness.messages.at(-1);

  harness.emit('webviewclosed', {
    response: JSON.stringify({ WEATHER_UPDATE_MINUTES: 15 }),
  });
  await settlePromises();

  assert.equal(harness.positions.length, 0);
  assert.equal(harness.requests.length, 2);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.get(timer[0]), timer[1]);
  assert.deepEqual(harness.messages.at(-1), cached);
});

test('closing configuration without saving leaves the refresh timer unchanged', async () => {
  const harness = createHarness();
  await harness.sync();
  await harness.completeRefresh();
  const timer = [...harness.timers.entries()][0];

  for (const event of [undefined, {}, { response: '' }]) {
    harness.emit('webviewclosed', event);
  }
  await settlePromises();

  assert.equal(harness.positions.length, 0);
  assert.equal(harness.messages.length, 1);
  assert.equal(harness.timers.size, 1);
  assert.equal(harness.timers.get(timer[0]), timer[1]);
});
