from dataclasses import dataclass
from pathlib import Path
from types import MappingProxyType
from typing import Any, Final

SUPPORT_MATRIX: Final = MappingProxyType(
    {
        "aplite":
        MappingProxyType(
            {
                "displays": ("white", "black"),
                "capabilities": ("weather", "battery", "location", "bluetooth"),
            }
        ),
        "basalt":
        MappingProxyType(
            {
                "displays": ("white", "black"),
                "capabilities": ("weather", "battery", "health", "location", "bluetooth", "all"),
            }
        ),
        "chalk":
        MappingProxyType(
            {
                "displays": ("white", "black", "celeste", "oxford"),
                "capabilities": ("weather", "battery", "health", "location", "bluetooth", "all"),
            }
        ),
        "diorite":
        MappingProxyType(
            {
                "displays": ("white", "black"),
                "capabilities": ("weather", "battery", "health", "location", "bluetooth", "all"),
            }
        ),
        "emery":
        MappingProxyType(
            {
                "displays": ("white", "black", "celeste", "oxford"),
                "capabilities": ("weather", "battery", "health", "location", "bluetooth", "all"),
            }
        ),
        "flint":
        MappingProxyType(
            {
                "displays": ("white", "black"),
                "capabilities": ("weather", "battery", "health", "location", "bluetooth", "all"),
            }
        ),
        "gabbro":
        MappingProxyType(
            {
                "displays": ("white", "black", "celeste", "oxford"),
                "capabilities": ("weather", "battery", "health", "location", "bluetooth", "all"),
            }
        ),
    }
)

SUPPORTED_CAPABILITIES: Final = frozenset(
    capability for support in SUPPORT_MATRIX.values() for capability in support["capabilities"]
)

GRAMMAR_VOCABULARY: Final = frozenset(
    {
        "PREAMBLE",
        "DISPLAYS",
        "EMULATORS",
        "SCREENSHOTS",
        "STEP",
        "EXECUTE",
        "END",
        "MEMBERS",
        "INCLUDE",
        "STEPS",
    }
)


@dataclass(frozen=True)
class QAPlanGrammar:
  SUPPORT_MATRIX: Final = SUPPORT_MATRIX
  SUPPORTED_CAPABILITIES: Final = SUPPORTED_CAPABILITIES
  GRAMMAR_VOCABULARY: Final = GRAMMAR_VOCABULARY
  SCREENSHOTS_POLICIES: Final = frozenset({"yes", "no"})
  SUPPORTED_EMULATORS: Final = frozenset(SUPPORT_MATRIX)
  ACCEPTED_PLAN_TYPES: Final = MappingProxyType(
      {
          ".scenario": "scenario",
          ".suite": "suite",
          ".matrix": "matrix",
      }
  )
  ACCEPTED_STEPS_SUFFIX: Final[str] = ".steps"
  EXPECTED_SCREENSHOTS: Final = MappingProxyType(
      {
          "weather": 1,
          "battery": 1,
          "health": 1,
          "location": 1,
          "bluetooth": 2,
          "all": 2,
      }
  )

  CAPABILITY_FIELDS: Final = MappingProxyType(
      {
          "weather":
          frozenset({"temp", "code", "is_day"}),
          "battery":
          frozenset({"level", "charging"}),
          "health":
          frozenset({"bpm", "steps"}),
          "location":
          frozenset({"location"}),
          "bluetooth":
          frozenset({"connected"}),
          "all":
          frozenset(
              {
                  "bpm",
                  "steps",
                  "temp",
                  "code",
                  "is_day",
                  "level",
                  "charging",
                  "location",
                  "connected",
              }
          ),
      }
  )

  @classmethod
  def does_step_have_needed_attributes(cls, capability: str, args: dict[str, Any]) -> bool:
    if not capability or capability not in cls.SUPPORTED_CAPABILITIES:
      raise ValueError(f"Unsupported capability '{capability}'")
    return cls.CAPABILITY_FIELDS[capability].issubset(set(args.keys()))

  @classmethod
  def is_valid_capability(cls, capability: str) -> bool:
    return capability in cls.SUPPORTED_CAPABILITIES

  @classmethod
  def is_valid_emulator(cls, emulator: str) -> bool:
    return emulator in cls.SUPPORT_MATRIX

  @classmethod
  def is_capability_supported_by_emulator(cls, capability: str, emulator: str) -> bool:
    return (
        emulator in cls.SUPPORT_MATRIX
        and capability in cls.SUPPORT_MATRIX[emulator]["capabilities"]
    )

  @classmethod
  def is_valid_display(cls, display: str) -> bool:
    return any(display in support["displays"] for support in cls.SUPPORT_MATRIX.values())

  @classmethod
  def is_valid_display_for_emulator(cls, display: str, emulator: str) -> bool:
    return (emulator in cls.SUPPORT_MATRIX and display in cls.SUPPORT_MATRIX[emulator]["displays"])


class AcceptedItem:
  pass


class DiscardedItem:
  pass


@dataclass
class ParsedStep(AcceptedItem):
  capability: str
  fields: dict[str, str]


@dataclass(frozen=True)
class AcceptedMember(AcceptedItem):
  kind: str
  name: str
  path: Path


@dataclass(frozen=True)
class ParseDiscard(DiscardedItem):
  line_number: int
  source: str
  reason: str

  def describe(self) -> str:
    return f"line {self.line_number}: {self.source} ({self.reason})"


@dataclass(frozen=True)
class MemberDiscard(DiscardedItem):
  kind: str
  name: str
  reason: str

  def describe(self) -> str:
    return f"{self.kind.title()} '{self.name}': {self.reason}"


def validate_slug(value: str, label: str) -> None:
  if not value or not value[0].isalnum():
    raise ValueError(f"Invalid {label} '{value}'")
  for char in value:
    if char.islower() or char.isdigit() or char == "_" or char == "-":
      continue
    raise ValueError(f"Invalid {label} '{value}'")
