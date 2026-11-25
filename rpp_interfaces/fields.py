"""Interpret the common field schema and build routes between layouts."""

from __future__ import annotations

from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from math import isfinite
from typing import ClassVar, TYPE_CHECKING

if TYPE_CHECKING:
    from rpp_schema.rpp_interfaces import FieldDesc

# Values from plugin_types/common/constants.capnp.
_EUCLIDEAN = 1
_CONTINUOUS_ANGLE = 2
_QUATERNION = 3
_INVALID_QUANTITY = 0
_ANGULAR_POSITION = 4
_ORIENTATION = 13


@dataclass(frozen=True)
class Field:
    """Immutable copy of a FieldDesc, using the schema's attribute names."""

    name: str
    kind: int
    quantity: int
    size: int = 0
    frameId: str = ""
    childFrameId: str = ""
    resource: str = ""


@dataclass(frozen=True)
class FieldRange:
    """Stored and tangent offsets and widths for one field."""

    offset: int
    size: int
    tangent_offset: int
    tangent_size: int


def _widths(field: Field) -> tuple[int, int]:
    if field.size < 0:
        raise ValueError("Field size must be nonnegative")
    if field.kind == _EUCLIDEAN:
        if field.size == 0:
            raise ValueError("Euclidean field size must be positive")
        return field.size, field.size
    if field.kind == _CONTINUOUS_ANGLE:
        if field.quantity != _ANGULAR_POSITION:
            raise ValueError("Continuous angles require angular position quantities")
        width = field.size or 1
        return width, width
    if field.kind == _QUATERNION:
        if field.quantity != _ORIENTATION:
            raise ValueError("Quaternions require orientation quantities")
        if field.size not in (0, 4):
            raise ValueError("Quaternion size must be 0 or 4")
        return 4, 3
    raise ValueError("Unsupported coordinate kind")


@dataclass(frozen=True, init=False)
class FieldLayout:
    """Validated layout built from a descriptor's ordered fields."""

    max_width: ClassVar[int] = 4096
    fields: tuple[Field, ...]
    ranges: tuple[FieldRange, ...]
    size: int
    tangent_size: int

    def __init__(self, fields: Iterable[Field | FieldDesc] = ()) -> None:
        prepared = tuple(
            Field(f.name, f.kind, f.quantity, f.size, f.frameId, f.childFrameId, f.resource)
            for f in fields
        )
        names: set[str] = set()
        ranges: list[FieldRange] = []
        size = tangent_size = 0
        for field in prepared:
            if not field.name or field.name in names or field.quantity == _INVALID_QUANTITY:
                raise ValueError("Fields require unique nonempty names and a quantity")
            names.add(field.name)
            width, tangent = _widths(field)
            if width > self.max_width - size:
                raise ValueError("State layout is too large")
            ranges.append(FieldRange(size, width, tangent_size, tangent))
            size += width
            tangent_size += tangent

        object.__setattr__(self, "fields", prepared)
        object.__setattr__(self, "ranges", tuple(ranges))
        object.__setattr__(self, "size", size)
        object.__setattr__(self, "tangent_size", tangent_size)

    def valid_row(self, values: Sequence[float], start: int = 0) -> bool:
        """Check the finite values and unit quaternions in one numeric row."""
        if start < 0 or start + self.size > len(values):
            return False
        if any(not isfinite(values[i]) for i in range(start, start + self.size)):
            return False
        for field, field_range in zip(self.fields, self.ranges):
            if field.kind == _QUATERNION:
                offset = start + field_range.offset
                norm = 0.0
                for i in range(offset, offset + 4):
                    value = float(values[i])
                    norm += value * value
                if not isfinite(norm) or abs(norm - 1.0) > 1e-3:
                    return False
        return True


def find_field(layout: FieldLayout, name: str) -> int | None:
    """Return the index of a named field, if present."""
    return next((i for i, field in enumerate(layout.fields) if field.name == name), None)


def same_field(a: FieldLayout, ai: int, b: FieldLayout, bi: int) -> bool:
    """Compare field meaning and effective stored width."""
    x, y = a.fields[ai], b.fields[bi]
    return (
        x.name == y.name
        and x.kind == y.kind
        and x.quantity == y.quantity
        and x.frameId == y.frameId
        and x.childFrameId == y.childFrameId
        and x.resource == y.resource
        and a.ranges[ai].size == b.ranges[bi].size
    )


def validate_required_fields(available: FieldLayout, required: FieldLayout) -> bool:
    """Check a required subset without building a route or raising on mismatch."""
    for index, field in enumerate(required.fields):
        found = find_field(available, field.name)
        if found is None or not same_field(available, found, required, index):
            return False
    return True


def field_route(source: FieldLayout, destination: FieldLayout) -> list[int]:
    """Return the source indices needed to populate a destination row."""
    route: list[int] = []
    for i, field in enumerate(destination.fields):
        found = find_field(source, field.name)
        if found is None or not same_field(source, found, destination, i):
            raise ValueError(f"Missing or incompatible field: {field.name}")
        source_range = source.ranges[found]
        route.extend(range(source_range.offset, source_range.offset + source_range.size))
    return route


def merge_fields(first: FieldLayout, second: FieldLayout) -> FieldLayout:
    """Append new fields from the second layout in their declared order."""
    fields = list(first.fields)
    for i, field in enumerate(second.fields):
        found = find_field(first, field.name)
        if found is None:
            fields.append(field)
        elif not same_field(first, found, second, i):
            raise ValueError(f"Conflicting field: {field.name}")
    return FieldLayout(fields)
