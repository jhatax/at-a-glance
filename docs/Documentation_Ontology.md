# Documentation Ontology

This ontology defines the irreducible knowledge domains for the At A Glance
documentation set.

## Documentation Invariants And Guardrails

*Core Documentation Principle*
One document, one concept, one responsibility, many references.

- Invariant: *one* first-class concept gets *one* canonical home. Collectively,
  the corpus is exhaustive and unambiguous.
- Be deliberate about content placement before adding to a document.
- Separate prescriptive docs (what) from evidentiary and implementation docs (how).
- Keep implementation docs source-backed and current to the live tree.
- Keep documents related, not prerequisite-dependent. Cross-reference freely.
- Each document must stand on its own inside its purpose.
- Move drift to the canonical home instead of duplicating or partially
  restating it elsewhere.
- Add a first-class concept only when it answers an independent question that
  no existing concept fully covers.
- Lightly summarize information in a related concept to improve usability. Treat repetition
  as an explicit exception, not a second source of truth.

## Canonical Knowledge Domains

Translation of guardrails: Before placing content, classify the knowledge using
these first-class concepts:

| Concept | Canonical Home | One-line blurb | Keywords |
|---|---|---|---|---|
| Product entry point | [Watchface_Readme](../README.md) | Introduces the watch face and gets a new user started. | product, screenshots, features, platforms, configuration, quickstart install and build |
| Product intent | [Design](Design.md) | Explains the design intent behind the product. | intent, motivations, design prescriptions, influences |
| Product invariants | [ProductInvariants](ProductInvariants.md) | Defines the product properties that must remain true. | acceptance properties, commitments, implementation-independent rules |
| Visual vocabulary | [VisualVocabulary](VisualVocabulary.md) | Defines the visual rules that make those properties recognizable. | visual rules, recognizability, glyph system, channels, silhouette |
| Source organization | [SourceMap](SourceMap.md) | Maps source files to their responsibilities and relationships. | source map, headers, modules, QA tree, ownership edges |
| Visual evidence | [UserInterface](UserInterface.md) | Records the current visual realization of the product. | visual details, information display, screenshots, geometry, fonts, palettes |
| Runtime architecture | [ArchitectureLedger](ArchitectureLedger.md) | Describes the current runtime structure and ownership boundaries. | implementation, runtime, ownership, boundaries, lifecycle, modules |
| Settings and configuration | [SettingsandConfiguration](SettingsandConfiguration.md) | Describes the settings path from configuration to runtime behavior. | settings, Clay, messageKeys, PKJS normalization, persistence, defaults, validation |
| Build system | [Build](BuildandInstall.md) | Describes build, install, and editor-tooling operations. | build flow, compile database generation, install flow, editor tooling, build-stage ownership |
| Validation system | [Validation](Validation.md) | Describes validation paths and the evidence each path produces. | validation flow, scenarios, runtime validation, artifacts, reports, visual review |
| Contributor workflow | [Contributing](Contributing.md) | Guides contributors through changes, validation, and review. | contributor workflow, review discipline, C-coding decisions, documentation QA |
| QA system architecture | [QA_Readme](../qa/README.md) | Explains how to operate the QA harness and inspect its artifacts. | harness flow, commands, artifacts, Python ownership, shell boundary |
| QA test cases and plans | [WritingTestCasesAndPlans](../qa/docs/WritingTestCasesAndPlans.md) | Shows how to write scenarios, suites, steps, and fixtures. | suites, scenarios, steps, grammar, fixtures |
| QA implementation flow | [QA_Harness_Implementation_Flow](../qa/docs/QA_Harness_Implementation_Flow.md) | Maps the live harness execution, inspection, and reporting flow. | functions, ownership, execution flow, inspection flow, validation gates |
| Documentation governance | [Documentation_Ontology](Documentation_Ontology.md) | Defines document ownership, relationships, and publishing rules. | ontology, canonical homes, document relationships, publishing |

## Prescriptive vs. Evidentiary Documents

The documentation intentionally separates product specification from
implementation.

[Watchface_Readme](../README.md) is the one audience exception. It is written for a casual user who
may never read another file in the repository, so it may lightly summarize
product behavior and include a small number of screenshots even when the
canonical details live elsewhere.

**Prescriptive documents**
These documents explain intent, define acceptance properties, and specify
visual rules.

1. [Design](Design.md)
2. [ProductInvariants](ProductInvariants.md)
3. [VisualVocabulary](VisualVocabulary.md)

**Evidentiary / implementation documents**
These documents demonstrate, implement, build, validate, and govern the current
product and its QA system.

1. [UserInterface](UserInterface.md)
2. [ArchitectureLedger](ArchitectureLedger.md)
3. [SourceMap](SourceMap.md)
4. [SettingsandConfiguration](SettingsandConfiguration.md)
5. [Build](BuildandInstall.md)
6. [Validation](Validation.md)
7. [Contributing](Contributing.md)
8. [QA_Readme](../qa/README.md)
9. [WritingTestCasesAndPlans](../qa/docs/WritingTestCasesAndPlans.md)
10. [QA_Harness_Implementation_Flow](../qa/docs/QA_Harness_Implementation_Flow.md)

## QA Documentation Connections

The QA documents form one connected subsystem with separate ownership:

- `docs/Validation.md` tells contributors which validation path to run.
- `docs/Contributing.md` tells contributors how to apply the validation and
  documentation workflow while changing the repository.
- `qa/README.md` explains the harness structure and public commands.
- `qa/docs/WritingTestCasesAndPlans.md` defines test-plan anatomy, grammar, and fixtures.
- `qa/QA_Harness_Implementation_Flow.md` maps live functions and execution flow.
- report payload and rendering rules belong in the implementation-flow section
  that documents the live reporting path.

Generated run artifacts under `qa/qa-runs/` are evidence, not documentation,
and are not part of the checked-in documentation corpus.

## Emergent Properties

The following are not first-class concepts because they carry no unique
knowledge:

- Product Identity
- Shared DNA
- Layout
- Display Modes
- Color
- Typography
- Platform Adaptation

They may appear where useful, but they stay embedded in the documents above
until one of them needs an independent question, audience, and maintenance
surface.
