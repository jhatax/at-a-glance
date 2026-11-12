# Native helper tests

Run the native test loop with:

```sh
tests/native/run.sh
```

The tests compile the production tuple parser and computation helpers against a small Pebble header stub. They run locally without a Pebble build or emulator. [Validation](../../docs/Validation.md) owns the broader validation paths.
