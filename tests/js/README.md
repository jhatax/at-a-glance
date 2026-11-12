# JavaScript tests

Run the JavaScript tests from the repository root:

```sh
npm run test:js
```

The tests use Node's built-in test runner and a shared harness to run production PKJS with simulated Pebble events, GPS, network responses, and timers. They run locally without a Pebble build or emulator. [Validation](../../docs/Validation.md) owns the broader validation paths.
