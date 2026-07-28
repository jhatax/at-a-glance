# Documentation Contracts

Defines the contract -- purpose, sub-concepts, connections -- fulfilled by every document in this documentation system.

## Documentation Details and Connections

### Watchface_Readme

**Purpose:** Introduces *At A Glance* to a new user. It provides the shortest path to screenshots, features, installation, and source build.

**Sub-concepts:** Product identity; screenshots; installation; build prerequisites; emulator installation; phone installation; configuration; supported platforms; feature list; next reading.

**Connections:** [UserInterface](UserInterface.md). Read next: [BuildandInstall](BuildandInstall.md), [Contributing.md](Contributing.md).

### DocumentationOntology

**Purpose:** Defines the canonical home for each knowledge domain. It keeps the corpus connected through ownership, document class, and navigation rules.

**Sub-concepts:** Documentation invariants; canonical knowledge domains; relationship graph.

**Connections:** [DocumentationOntology](DocumentationOntology.md) owns canonical-domain classification; this document owns the relationship map.

### DocumentationContracts (this)

**Purpose:** Defines the purpose, sub-concepts, and navigation contract for every document. It gives contributors the durable procedure for adding a document without weakening the documentation system.

**Sub-concepts:** Document purpose; concept boundaries; canonical ownership; adjacent documents; read-next documents; document-addition procedure; contract rules.

**Connections:** Adjacent: [DocumentationOntology](DocumentationOntology.md). Read next: [Contributing](Contributing.md).

### Design

**Purpose:** Defines why *At A Glance* looks and behaves as a glance-first watch face. It establishes the product motivation, visual calm, cross-device intent, and automatic-perception invariant.

**Sub-concepts:** Product intent; design influences; glance-first philosophy; calm and negative space; intentional device support; shared visual DNA; design invariant.

**Connections:** Adjacent: none. Read next: [ProductInvariants](ProductInvariants.md), [VisualVocabulary](VisualVocabulary.md).

### ProductInvariants

**Purpose:** Defines the product properties that every implementation must preserve. It turns design intent into observable acceptance properties across devices and display families.

**Sub-concepts:** Glanceability; automatic perception; first-class device support; information hierarchy; dominant time; identifiable visual elements; semantic channel meaning; color meaning; out-of-range perception.

**Connections:** Adjacent: [Design](Design.md), [VisualVocabulary](VisualVocabulary.md). Read next: [RuntimeArchitecture](RuntimeArchitecture.md), [UserInterface](UserInterface.md).

### VisualVocabulary

**Purpose:** Defines the visual rules used to satisfy the product invariants. It assigns responsibilities to channels and establishes recognition, composition, glyph, stroke, color, display-mode, and out-of-range rules.

**Sub-concepts:** Visual channels; immediate identifiability; surface composition; glyph and icon selection; icon recognizability; silhouette; climate glyphs; stroke and simplification; stable footprint; color as state support; display modes; palettes; typography; progress bars; unavailable states; visual validation.

**Connections:** Adjacent: [ProductInvariants](ProductInvariants.md). Read next: [UserInterface](UserInterface.md).

### UserInterface

**Purpose:** Records the current visual realization of the watch face. It provides the evidence layer for stack order, geometry, typography, palettes, icons, glyphs, and settings presentation.

**Sub-concepts:** Device screenshots; settings screenshot; visual stack; information placement; typography; color and palettes; progress tracks; icon frames; glyph inventory; weather glyphs; out-of-range slash.

**Connections:** Adjacent: [ProductInvariants](ProductInvariants.md), [VisualVocabulary](VisualVocabulary.md). Read next: [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md), [SettingsandConfiguration](SettingsandConfiguration.md), [Validation](Validation.md).

### RuntimeArchitecture

**Purpose:** Describes the current runtime structure and ownership boundaries. It follows initialization, event, settings, module, palette, transport, capability, and lifecycle flows through the watch face.

**Sub-concepts:** Conceptual layers; initialization flow; runtime event flow; settings persistence; weather-cadence sync; prepared surface; architect; stylist; runtime owner; module lifecycle; required strata; module boundaries; steps module; palette resolution; capability guards; AppMessage coverage; lifecycle priorities.

**Connections:** Adjacent: [ProductInvariants](ProductInvariants.md). Read next: [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md), [SettingsandConfiguration](SettingsandConfiguration.md), [Validation](Validation.md).

### WatchfaceImplementationFlow

**Purpose:** Maps implementation files and shared headers to their responsibilities. It gives a contributor a source-level route from manifest and build inputs through runtime ownership and feature modules.

**Sub-concepts:** Feature and documentation folders; feature source; documentation locations; manifest and message keys; PKJS; build script; Pebble lifecycle adapter; runtime boundary; watchface owner; layout facade; surface and style vocabulary; layout delegates; feature modules; renderer; helper; header groups; runtime relationship; debug source placement.

**Connections:** Adjacent: [RuntimeArchitecture](RuntimeArchitecture.md), [UserInterface](UserInterface.md), [SettingsandConfiguration](SettingsandConfiguration.md). Read next: [Contributing](Contributing.md).

### SettingsandConfiguration

**Purpose:** Defines the settings contract from Clay and PKJS through AppMessage, C parsing, persistence, and runtime effects. It records defaults, valid values, message keys, and the validation obligations for changes.

**Sub-concepts:** Setting catalog; defaults; valid ranges; Clay mapping; messageKeys; PKJS normalization; persisted and ephemeral values; runtime side effects; AppMessage payloads; message-key table; persistence; validation requirements; change checklist.

**Connections:** Adjacent: [ProductInvariants](ProductInvariants.md), [VisualVocabulary](VisualVocabulary.md). Read next: [RuntimeArchitecture](RuntimeArchitecture.md), [UserInterface](UserInterface.md), [BuildandInstall](BuildandInstall.md), [Validation](Validation.md).

### Contributing

**Purpose:** Guides contributors from repository setup through build, validation, review, formatting, and commit. It provides the working discipline that keeps product, source, and documentation changes traceable.

**Sub-concepts:** GitHub checkout; prerequisites; Pebble SDK; environment verification; Git hook setup; first build; IDE support; build/install/QA automation; C guidelines; runtime logs; debugging; glyph development; repository maintenance; review discipline; formatters; pre-commit; documentation management.

**Connections:** Adjacent: [BuildandInstall](BuildandInstall.md), [Validation](Validation.md). Read next: [RuntimeArchitecture](RuntimeArchitecture.md), [SettingsandConfiguration](SettingsandConfiguration.md), [QA_Readme](../qa/README.md).

### BuildandInstall

**Purpose:** Describes build, compile-database, emulator-install, device-install, and recovery flows. It keeps build-stage ownership and editor tooling separate from product and QA semantics.

**Sub-concepts:** Build goals; build rules; tooling surface; Pebble build; compile database; emulator install; harness install; emulator recovery; phone install; build artifacts.

**Connections:** Adjacent: [Contributing](Contributing.md), [Validation](Validation.md). Read next: none.

### Validation

**Purpose:** Defines how contributors choose a validation path and what evidence each path produces. It keeps signoff with the operator while making build, runtime, settings, visual, and release checks repeatable.

**Sub-concepts:** Validation goals; validation invariants; path selection; operator evidence; release validation; canonical pre-release plan; scenario execution; run inspection; comparison; manual emulator commands; battery; weather; display mode; one-shot health; config page; debug builds.

**Connections:** Adjacent: [ProductInvariants](ProductInvariants.md), [Contributing](Contributing.md), [BuildandInstall](BuildandInstall.md). Read next: [QA_Readme](../qa/README.md).

### QA_Readme

**Purpose:** Explains how to operate the QA harness and inspect its artifacts. It presents the public command flow, shell/Python boundary, run outputs, and correctness-test command.

**Sub-concepts:** Public entrypoint; operator command flow; shell/Python boundary; scenario and suite operations; run artifacts; report inspection; comparison; build handoff; correctness tests; fixtures.

**Connections:** Adjacent: [Validation](Validation.md), [Contributing](Contributing.md). Read next: [WritingTestCasesAndPlans](../qa/docs/WritingTestCasesAndPlans.md), [QAHarnessImplementationFlow](../qa/docs/QAHarnessImplementationFlow.md).

### WritingTestCasesAndPlans

**Purpose:** Shows operators how to write the narrow QA grammar. It explains plan anatomy, scenario steps, suite composition, supported fields, policy rules, and grammar fixtures.

**Sub-concepts:** Scenario example; suite example; plan hierarchy; step construction; scenario construction; suite construction; file shapes; grammar rules; capabilities; required fields; unordered field arguments; weather; battery; health; screenshot policy; identity and de-duplication; fixtures.

**Connections:** Adjacent: [QA_Readme](../qa/README.md), [Validation](Validation.md). Read next: [QAHarnessImplementationFlow](../qa/docs/QAHarnessImplementationFlow.md).

### QAHarnessImplementationFlow

**Purpose:** Maps the live QA harness from shell handoff through Python dispatch, plan resolution, step execution, finalization, inspection, and comparison. It records function ownership and the canonical JSON-to-Markdown report boundary.

**Sub-concepts:** Implementation guardrails; four runtime phases; parser handoff; resolver handoff; executor handoff; finalization handoff; function ownership; Pebble adapter boundary; canonical JSON report; Markdown rendering; inspection and comparison; harness-specific validation gates.

**Connections:** Adjacent: [QA_Readme](../qa/README.md), [WritingTestCasesAndPlans](../qa/docs/WritingTestCasesAndPlans.md), [Validation](Validation.md). Read next: [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md).

## Adding A Document

1. State the document's independent question in one sentence.
2. Assign one canonical purpose and list the sub-concepts it owns.
3. Classify the document as prescriptive, evidentiary, or implementation.
4. Identify immediate conceptual neighbors under `Adjacent`.
5. Identify documents that apply or demonstrate its ideas under `Read Next`.
6. Keep `Adjacent` and `Read Next` mutually exclusive.
7. Add the document to `DocumentationOntology.md` and this contract map.
8. Link to the canonical owner instead of duplicating its content elsewhere.

## Contract Rules

- One document owns each first-class concept.
- A document's opening purpose states the question it answers.
- The sub-concept list stays within that purpose.
- Prescriptive documents define intent, acceptance properties, or visual rules.
- Evidentiary, implementation, and validation documents describe the current product and supporting systems.
- Implementation claims follow the live source tree.
- Each document stands on its own inside its purpose.
- `Adjacent` names immediate conceptual neighbors.
- `Read Next` names documents that apply or demonstrate the current document's ideas.
- Navigation links use repository-relative targets and visible document names.
