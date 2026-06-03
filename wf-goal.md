# Watchface Goal

Your Goal:

Guiding principles: Review code using a review capability or skill. Be smart
about extracting helpers, but do not create helpers just because basic
arithmetic calculations are tedious. Think like an architect, plan every
change, then write code. Resist the impulse to jump into churning out code.
The work is better when it is deliberate.

Keep slices small. Audit, commit, build, commit the change, then move to the
next.

## Weather Condition Icons

1. Finish the current weather-condition icon pass.
   - Continue the approach refined through the latest iterations.

2. Keep the lightweight glyph approach using private static inlines.
   - Confirm the build.
   - Commit this change as the weather addition.

3. Commit the changes to `main.c` with the established commit-message style:
   - bulleted list
   - precise feature in the header
   - key functions added or changed
   - functionality modified

## Round Watchface Planning

1. Build a plan based on the SDK, Pebble documentation guidelines, and
   community work on round watchfaces.
   - The new SDK and developer documentation covers challenges with rendering
     text on round displays, including clipping.
   - Factor these constraints in before repositioning the four rows of
     content in the watchface.

## Main Module Planning

1. Review `main.c` and build a plan for how many modules can simplify it in
   the same way as the weather module.
   - Identify dependencies and points of fracture in the flow and logic.
   - Use this to unwind globals deliberately.
   - Move pure helpers or library functions into a library module when the
     ownership boundary is real.

## Doing

1. Delete or decide what to do with untracked screenshots.
2. Delete or decide whether `resources/images/rocket.pdc` should be removed
   or kept untracked.

## State, Settings, And Weather Fallback Planning

Build a plan, describe available actions, and prepare a patch that does each
of these:

1. Split state into explicit structs where it improves clarity.

2. Review and propose changes to persisted settings struct tolerance in
   `settings_load()`.
   - Take cues from GitHub watchfaces with many users, like TimeStyle.

3. Investigate phone weather fallback configurability and propose a path
   forward.
