# Project Rename Plan: `at-a-glance`

This plan documents the work needed to rename the project folder from
`life-at-a-glance-emery-wf` to `at-a-glance`.

Do not perform this rename as an incidental cleanup. It changes local paths,
generated artifact names, and likely publishing metadata.

## Folder Rename

Target local folder:

```text
~/Code/at-a-glance
```

Required local steps:

1. Ensure the worktree is clean.
2. Close running Pebble emulator/install processes that reference the old
   path.
3. Rename the folder from outside the repo:
   ```sh
   mv ~/Code/life-at-a-glance-emery-wf ~/Code/at-a-glance
   ```
4. Reopen the project in the new path.
5. Rebuild from the new path.

## Source And Build Ramifications

### `wscript`

The current `wscript` uses relative source globs:

```python
src/c/**/*.c
src/modules/**/*.c
src/pkjs/**/*.js
src/pkjs/**/*.json
```

The folder rename should not require source-path changes in `wscript`.

### `package.json`

Review these fields:

- `name`: currently `AtAGlance`
- `pebble.displayName`: currently `At A Glance`
- `description`
- `keywords`
- `version`

If the product name remains `At A Glance`, the folder rename does not require
a display-name change. If the package identity should match the folder, decide
whether `name` should become `at-a-glance`.

Do not change `pebble.uuid` for a rename unless this is intended to become a
new app listing rather than an update to the same app.

### PebbleKit JS

`src/pkjs/index.js` does not currently embed the repo folder name. It uses
relative Clay config imports and AppMessage keys, so the folder rename should
not affect runtime JS.

### Generated PBW

The PBW filename is derived by the Pebble build tooling from project metadata
and/or folder context. After renaming, confirm the generated `build/*.pbw`
filename and update README/App Store notes accordingly.

### Documentation

Update path references in:

- `README.md`
- `agents.md`
- `ARCHITECTURE_LEDGER.md`
- `appstore-submission.md`
- any handoff or retrospective document that still names the old folder as
  active context

Historical docs may keep old path names if the path is part of the record.

## Validation

After the rename:

```sh
npm install
pebble build
pebble install --emulator emery
```

Run at least one additional target if the rename is paired with a release
candidate.
