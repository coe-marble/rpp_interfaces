"""Small examples of preparing layouts and routing flat numeric values."""

import pytest

from rpp_interfaces import Field, FieldLayout, field_route, validate_required_fields

# Coordinate and quantity values from plugin_types/common/constants.capnp.
EUCLIDEAN = 1
QUATERNION = 3
LINEAR_POSITION = 1
ORIENTATION = 13


def test_route_selects_fields_in_destination_order():
    position = Field("base.position", EUCLIDEAN, LINEAR_POSITION, 2, "map")
    orientation = Field("base.orientation", QUATERNION, ORIENTATION, frameId="map")
    source = FieldLayout([position, orientation])
    destination = FieldLayout([orientation, position])

    assert source.size == 6
    assert source.tangent_size == 5
    assert validate_required_fields(source, destination)
    route = field_route(source, destination)
    assert route == [2, 3, 4, 5, 0, 1]

    measured = [10.0, 20.0, 0.0, 0.0, 0.0, 1.0]
    assert [measured[index] for index in route] == [0.0, 0.0, 0.0, 1.0, 10.0, 20.0]


def test_route_rejects_an_incompatible_frame():
    available = FieldLayout([Field("base.position", EUCLIDEAN, LINEAR_POSITION, 2, "map")])
    required = FieldLayout([Field("base.position", EUCLIDEAN, LINEAR_POSITION, 2, "odom")])

    assert not validate_required_fields(available, required)
    with pytest.raises(ValueError, match="Missing or incompatible field: base.position"):
        field_route(available, required)


def test_row_requires_finite_values_and_a_unit_quaternion():
    layout = FieldLayout([Field("base.orientation", QUATERNION, ORIENTATION)])

    assert layout.valid_row([0.0, 0.0, 0.0, 1.0])
    assert not layout.valid_row([0.0, 0.0, 0.0, 2.0])
    assert not layout.valid_row([0.0, 0.0, float("nan"), 1.0])
    assert not layout.valid_row([0.0, 0.0, 1.0])
