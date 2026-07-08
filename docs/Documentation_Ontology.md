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

| Concept | Canonical Home | Core Question | Keywords |
|---|---|---|---|---|
| Product entry point | [README.md](../README.md) | What is this product, what does it look like, and how does someone get started? | product, screenshots, features, platforms, configuration, quickstart install and build |
| Product intent | [Design.md](Design.md) | Why is the product designed this way? | intent, motivations, design prescriptions, influences |
| Product invariants | [ProductInvariants.md](ProductInvariants.md) | What must remain true regardless of implementation? | acceptance properties, commitments, implementation-independent rules |
| Visual vocabulary | [VisualVocabulary.md](VisualVocabulary.md) | What rules should you follow to satisfy visual product invariants? | visual rules, recognizability, glyph system, channels, silhouette |
| Visual evidence | [UserInterface.md](UserInterface.md) | What does the current product look like? | visual details, information display, screenshots, geometry, fonts, palettes |
| Runtime architecture | [ArchitectureLedger.md](ArchitectureLedger.md) | How is the current watch face implementation organized? | implementation, runtime, ownership, boundaries, lifecycle, modules |
| Source organization | [SourceMap.md](SourceMap.md) | Which source files exist, and how do they relate? | source map, headers, modules, QA tree, ownership edges |
| Settings and configuration | [Settings_and_Configuration.md](Settings_and_Configuration.md) | How do settings move from Clay through PKJS, AppMessage, C, persistence, and runtime effects? | settings, Clay, messageKeys, PKJS normalization, persistence, defaults, validation |
| Tooling and validation implementation | [Build_and_Validation.md](Build_and_Validation.md) | How is the project built, installed, and validated operationally? | build flow, compile database generation, install flow, validation flow, tooling ownership |
| Contributor workflow | [Contributing.md](Contributing.md) | How should contributors change the project safely? | contributor workflow, review discipline, C-coding decisions, documentation QA |

## Prescriptive vs. Evidentiary Documents

The documentation intentionally separates product specification from
implementation.

[README.md](../README.md) is the one audience exception. It is written for a casual user who
may never read another file in the repository, so it may lightly summarize
product behavior and include a small number of screenshots even when the
canonical details live elsewhere.

Prescriptive documents:

1. [Design.md](Design.md)
2. [ProductInvariants.md](ProductInvariants.md)
3. [VisualVocabulary.md](VisualVocabulary.md)

These documents explain intent, define acceptance properties, and specify
visual rules. They leave implementation details to the following
**Evidentiary / implementation** documents:

1. [UserInterface.md](UserInterface.md)
2. [ArchitectureLedger.md](ArchitectureLedger.md)
3. [SourceMap.md](SourceMap.md)
4. [Settings_and_Configuration.md](Settings_and_Configuration.md)
5. [Build_and_Validation.md](Build_and_Validation.md)
6. [Contributing.md](Contributing.md)

These documents demonstrate, implement, build, validate, and govern the current
product.

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
