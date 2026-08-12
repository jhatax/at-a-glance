const APP_MESSAGE_RETRY_DELAY_MS = 1 * 1000;
const APP_MESSAGE_MAX_RETRIES = 5;

function createAppMessageQueue(transport) {
  var queue = [];
  var inFlight = false;
  var retryTimer = null;

  function pump() {
    if (inFlight || retryTimer !== null || queue.length === 0) {
      return;
    }

    var message = queue[0];
    inFlight = true;

    transport(
      message.payload,
      function () {
        queue.shift();
        inFlight = false;
        try {
          if (message.success) {
            message.success();
          }
        } finally {
          pump();
        }
      },
      function (error) {
        inFlight = false;
        if (message.retries < APP_MESSAGE_MAX_RETRIES) {
          ++message.retries;
          retryTimer = setTimeout(function () {
            retryTimer = null;
            pump();
          }, APP_MESSAGE_RETRY_DELAY_MS * message.retries); // Backoff to allow system to stabilize
          return;
        }

        queue.shift();
        try {
          if (message.failure) {
            message.failure(error);
          }
        } finally {
          pump();
        }
      }
    );
  }

  return function (payload, success, failure) {
    queue.push({
      payload: payload,
      success: success,
      failure: failure,
      retries: 0,
    });
    pump();
  };
}

module.exports = {
  createAppMessageQueue: createAppMessageQueue,
};
