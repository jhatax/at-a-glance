# Documentation Ontology

This ontology indexes the irreducible knowledge domains for the *At A Glance* documentation set.

## Ontology Invariants

1. One first-class concept has one canonical home.
2. The documentation set is complete when its first-class concepts have one clear owner.

Document purpose, sub-concepts, relationships, navigation, and document-addition rules belong in [DocumentationContracts](DocumentationContracts.md).

## Canonical Knowledge Domains

Translation of guardrails: Before placing content, classify the knowledge using these first-class concepts:

| Concept | Canonical Home |
|:---|:---|
| Product entry point | [Watchface_Readme](../README.md) |
| <b>Prescriptive Documents</b> | |
| Product intent | [Design](Design.md) |
| Product invariants | [ProductInvariants](ProductInvariants.md) |
| Visual vocabulary | [VisualVocabulary](VisualVocabulary.md) |
| <b>Implementation & Evidentiary Documents</b> | |
| Watchface implementation flow | [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md) |
| Visual evidence | [UserInterface](UserInterface.md) |
| Runtime architecture | [RuntimeArchitecture](RuntimeArchitecture.md) |
| Settings and configuration | [SettingsandConfiguration](SettingsandConfiguration.md) |
| Contributor workflow | [Contributing](Contributing.md) |
| Build & Install system | [BuildandInstall](BuildandInstall.md) |
| <b>Validation & QA Documents</b> | |
| Validation system | [Validation](Validation.md) |
| QA system architecture | [QA_Readme](../qa/README.md) |
| QA test cases and plans | [WritingTestCasesAndPlans](../qa/docs/WritingTestCasesAndPlans.md) |
| QA implementation flow | [QAHarnessImplementationFlow](../qa/docs/QAHarnessImplementationFlow.md) |
| <b>Governance Documents</b> | |
| Documentation routing | [DocumentationOntology](DocumentationOntology.md) |
| Documentation contracts | [DocumentationContracts](DocumentationContracts.md) |

## Document Relationship Graph

```mermaid
graph TD
  L1 ~~~ L2 ~~~ L3 ~~~ L4 ~~~ L5

  subgraph L1 ["<b>Design & Motivation</b>"]
    p1["Design"] ==> p2["Product Invariants"]
    p2 ==> p3["Visual Vocabulary"]
  end

  subgraph L2 ["<b>Implementation</b>"]
    p3 ==> i2["Runtime Architecture"]
    i2 ==> i3["Watchface Flow"]
    i3 ==> i4["User Interface"]
    i3 ==> c2["Settings & Config"]
    i4 ==> c2
  end

  subgraph L3 ["<b>Contributor Workflows</b>"]
    c2 ==> r1 ==> r2["Build & Install"] ==> r3["Validation"]
  end

  subgraph L4 ["<b>Validation & QA</b>"]
    r3 ==> q2["QA Readme"] ==> q3["Test Cases & Plans"] ==> q4["QA Harness Flow"]
  end

  subgraph L5 ["<b>Documentation Governance</b>"]
    d1["Docs Ontology"] ==> d2["Docs Contracts"] ==> r1["Contributing"]
    p2 ==> d1
  end
```

## Emergent Properties

The following are not first-class concepts because they carry no unique knowledge:

- Product Identity
- Shared DNA
- Layout
- Display Modes
- Color
- Typography
- Platform Adaptation

They may appear where useful, but they stay embedded in the documents above until one of them needs an independent question, audience, and maintenance surface.
