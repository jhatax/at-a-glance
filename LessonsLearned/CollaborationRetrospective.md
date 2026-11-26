# Collaboration retrospective

Teamwork. Agreed November 25, 2026, after reviewing the November 23–25 watchface work.

The native suite passed functional checks while unnecessary boundaries, repeated compilation, and unclear terminology obscured its flow. Completion requires architectural review as well as passing tests. Learning and useful refinement remain valuable parts of the work.

## Twelve rules

1. **Comprehension is a design invariant:** Manoj must understand, maintain, and explain the whole system six months later. Delegation preserves this requirement.
2. Start tooling design with the operator's normal workflow and required output. Before coding, confirm the intended change, tests, expected output, interaction model, defaults, and integration with existing tooling. Settle ownership and terminology.
3. Justify every abstraction, file, dependency, and configuration with a current need.
4. Show one complete execution path before generalizing or adding extension points.
5. Repeated confusion triggers design review before more explanation or documentation.
6. Trace actual callers before declaring a convention necessary.
7. Validate findings against reachable triggers, consequences, and accepted recovery behavior.
8. Measure total operator and developer effort: execution, diagnosis, repair, learning, and maintenance.
9. Build experimental machinery for reuse in implementation and extension; carry proven machinery forward.
10. Challenge instructions when they conflict with the user's stated goal.
11. Preserve documentation contracts alongside implementation and test evidence.
12. Stop adding machinery when its measurable benefit is negligible.

## Other agreements

- Judge implementations and criticism independently against first principles and evidence.
- Inspect existing architecture and reuse before introducing machinery; carry principles across code, tooling, QA, and documentation.
- Explain unfamiliar tools through one complete example. Remember declared knowledge gaps without repeated reminders.
- Learn by typing: contract → flow → small executable example → predict and test deliberate failures → production → regression coverage. The collaborator introduces suitable tools.
- Design verification for maintained suites. Controlled mocks need meaningful expectations. Decide what temporary investigations retain.
- Complete only after reviewing correctness, comprehensibility, verification, documentation, and final diffs. State remaining evidence limits.
- Keep prose concise and connected. Value productive learning and refinement; elapsed time alone is insufficient.

[Promise request chaining](PromiseRequestChaining.md) demonstrates completion and ordering. [Python QA harness lessons](PythonQAHarness.md) traces ownership and execution boundaries.
