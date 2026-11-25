const APP_MESSAGE_RETRY_DELAY_MS = 1 * 1000;
const APP_MESSAGE_MAX_ATTEMPTS = 6;

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
    ++message.attempts;
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
        if (message.attempts < message.maxAttempts) {
          retryTimer = setTimeout(function () {
            retryTimer = null;
            pump();
          }, APP_MESSAGE_RETRY_DELAY_MS * message.attempts); // Backoff to allow system to stabilize
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

  return (payload, success, failure, maxAttempts) => {
    queue.push({
      payload: payload,
      success: success,
      failure: failure,
      attempts: 0,
      maxAttempts: maxAttempts ? maxAttempts : APP_MESSAGE_MAX_ATTEMPTS,
    });
    pump();
  };
}

module.exports = {
  createAppMessageQueue: createAppMessageQueue,
};
