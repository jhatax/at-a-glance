from dataclasses import dataclass
from types import MappingProxyType
from typing import Any, Final
from qaharnessconfig import DISPLAY_MODE_VALUES


@dataclass(frozen=True)
class QAPlanGrammar:
  SUPPORTED_CAPABILITIES: Final = frozenset({"weather", "battery", "health", "all"})
  SCREENSHOTS_POLICIES: Final = frozenset({"yes", "no"})
  SUPPORTED_EMULATORS: Final = frozenset(
      {
          "aplite",
          "basalt",
          "chalk",
          "diorite",
          "emery",
          "flint",
          "gabbro",
      }
  )
  EMULATORS_SUPPORTING_HEALTH: Final = frozenset(
      {
          "basalt",
          "chalk",
          "diorite",
          "emery",
          "flint",
          "gabbro",
      }
  )

  COLOR_EMULATORS: Final = frozenset({"chalk", "emery", "gabbro"})
  ACCEPTED_PLAN_TYPES: Final = MappingProxyType({
      ".scenario": "scenario",
      ".suite": "suite",
  })
  DISPLAY_MODES_BY_EMULATOR: Final = MappingProxyType(
      {
          "color": frozenset(DISPLAY_MODE_VALUES),
          "monochrome": frozenset({"white", "black"}),
      }
  )
  EMULATOR_SUPPORT_BY_CAPABILITY: Final = MappingProxyType(
      {
          "health": frozenset(EMULATORS_SUPPORTING_HEALTH),
          "all": frozenset(EMULATORS_SUPPORTING_HEALTH),
          "battery": frozenset(SUPPORTED_EMULATORS),
          "weather": frozenset(SUPPORTED_EMULATORS),
      }
  )

  CAPABILITY_FIELDS: Final = MappingProxyType(
      {
          "weather":
          frozenset({"display", "temp", "code", "is_day"}),
          "battery":
          frozenset({"display", "level", "charging"}),
          "health":
          frozenset({"display", "bpm", "steps"}),
          "all":
          frozenset({
              "display",
              "bpm",
              "steps",
              "temp",
              "code",
              "is_day",
              "level",
              "charging",
          }),
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
    return emulator in cls.SUPPORTED_EMULATORS

  @classmethod
  def is_capability_supported_by_emulator(cls, capability: str, emulator: str) -> bool:
    return emulator in cls.EMULATOR_SUPPORT_BY_CAPABILITY[capability]

  @classmethod
  def are_valid_emulators(cls, emulators: list[str]) -> bool:
    return set(emulators).issubset(cls.SUPPORTED_EMULATORS)

  @classmethod
  def is_valid_display_for_emulator(cls, display: str, emulator: str) -> bool:
    return display in cls.DISPLAY_MODES_BY_EMULATOR["color" if emulator in
                                                    cls.COLOR_EMULATORS else "monochrome"]


@dataclass
class ParsedStep:
  capability: str
  needs_screenshot: bool
  fields: dict[str, str]


@dataclass
class ParsedScenario:
  name: str
  screenshots_policy: str
  emulators: list[str]
  steps: list[ParsedStep]


@dataclass
class ParsedSuite:
  name: str
  members: list[ParsedScenario]


def validate_slug(value: str, label: str) -> None:
  if not value or not value[0].isalnum():
    raise ValueError(f"Invalid {label} '{value}'")
  for char in value:
    if char.islower() or char.isdigit() or char == "_" or char == "-":
      continue
    raise ValueError(f"Invalid {label} '{value}'")
