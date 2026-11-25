// Clay stuff
var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var messageKeys = require('message_keys');
var createAppMessageQueue = require('./app_message_queue').createAppMessageQueue;
var createRefreshController = require('./weather_location').createRefreshController;

const STEPS_GOAL_MIN = 4000;
const STEPS_GOAL_DEFAULT = 10000;
const STEPS_GOAL_MAX = 32000;

// 15-minute default
const WEATHER_UPDATE_MINUTES_DEFAULT = 15;

// Max-update interval is 6-hours
const WEATHER_UPDATE_MINUTES_MIN = 15;
const WEATHER_UPDATE_MINUTES_MAX = 60;
function sendPebbleMessage(payload, success, failure) {
  Pebble.sendAppMessage(payload, success, failure);
}

var updateUser = createAppMessageQueue(sendPebbleMessage);
var updateClimateLocation = createAppMessageQueue(sendPebbleMessage);
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });
var kickoffUpdates = createRefreshController(updateClimateLocation);
var health_capable = supportsHealth();

function supportsHealth() {
  var watch = Pebble.getActiveWatchInfo ? Pebble.getActiveWatchInfo() : null;

  if (!watch || !watch.firmware) {
    return false;
  }

  var firmware = watch.firmware;

  return (
    watch.platform !== 'aplite' &&
    (firmware.major > 3 || (firmware.major === 3 && firmware.minor >= 8))
  );
}

function clampStepsGoal(goal) {
  if (goal < STEPS_GOAL_MIN) {
    return STEPS_GOAL_MIN;
  }
  if (goal > STEPS_GOAL_MAX) {
    return STEPS_GOAL_MAX;
  }
  return goal;
}

function calculateStepsGoal(settings) {
  var goal = STEPS_GOAL_DEFAULT;
  if (settings) {
    var presetGoal = parseInt(settings[messageKeys.STEPS_GOAL_PRESET], 10);
    var customGoal = parseInt(settings[messageKeys.STEPS_GOAL_CUSTOM], 10);

    if (!isNaN(customGoal)) {
      goal = clampStepsGoal(customGoal);
    } else if (!isNaN(presetGoal)) {
      goal = clampStepsGoal(presetGoal);
    }
  }

  return goal;
}

// Return the default value if setting is invalid or irretrievable
function validateWeatherUpdateMinutes(input) {
  var toReturn = parseInt(input, 10);
  toReturn = isNaN(toReturn) ? WEATHER_UPDATE_MINUTES_DEFAULT : toReturn;
  toReturn = toReturn <= WEATHER_UPDATE_MINUTES_MAX ? toReturn : WEATHER_UPDATE_MINUTES_MAX;
  return toReturn >= WEATHER_UPDATE_MINUTES_MIN ? toReturn : WEATHER_UPDATE_MINUTES_MIN;
}

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('appmessage', (e) => {
  if (e && e.payload && e.payload.SYNC === 1) {
    // Synchronization completed; start refresh
    var savedSettings = null;
    var minutes = WEATHER_UPDATE_MINUTES_DEFAULT;
    try {
      savedSettings = JSON.parse(localStorage.getItem('clay-settings'));
      minutes = validateWeatherUpdateMinutes(
        savedSettings ? savedSettings.WEATHER_UPDATE_MINUTES : undefined
      );
    } catch (e) {
      console.log('Parsing Clay Settings returned error: ' + e.message);
    }
    kickoffUpdates(minutes);
  }
});

Pebble.addEventListener('webviewclosed', (e) => {
  if (!e || !e.response) {
    return;
  }

  // Retrieves settings as a dictionary with message_keys
  // Ready to send via Pebble's app message to the watch face
  // so, messageKeys.KEY_NAME
  var dict = clay.getSettings(e.response);

  if (health_capable) {
    dict[messageKeys.STEPS_GOAL] = calculateStepsGoal(dict);
  } else {
    delete dict[messageKeys.STEPS_GOAL];
    delete dict[messageKeys.HR_SAMPLE_MINUTES];
  }
  // Always delete these keys
  delete dict[messageKeys.STEPS_GOAL_PRESET];
  delete dict[messageKeys.STEPS_GOAL_CUSTOM];
  dict[messageKeys.WEATHER_UPDATE_MINUTES] = validateWeatherUpdateMinutes(
    dict[messageKeys.WEATHER_UPDATE_MINUTES]
  );
  updateUser(
    dict,
    () => console.log('Sent config data to Pebble'),
    (e) => console.log('Config send failed with error: (' + e.code + '): ' + e.message)
  );
  // Update interval might have changed; so kickoff updates
  kickoffUpdates(dict[messageKeys.WEATHER_UPDATE_MINUTES]);
});

Pebble.addEventListener('ready', function () {
  try {
    updateUser({ SYNC: 1 });
  } catch (e) {
    console.log('Ready Event Handler failed due to error: ' + e.message);
  }
});
