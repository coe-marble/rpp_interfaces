import pytest

from rpp_interfaces import Field, FieldLayout
from rpp_interfaces.parameter_helpers import get_velocity_reference


@pytest.mark.parametrize("dimensions,width", [(2, 6), (3, 12)])
def test_velocity_targets_have_body_frames_and_no_pose_fields(dimensions, width):
    reference = get_velocity_reference(
        resource="vehicle", dimensions=dimensions, frame_id="body", inner_controller="ctl1")
    fields = reference["fields"]
    assert [field["name"] for field in fields] == [
        "vehicle.linearVelocity", "vehicle.angularVelocity",
        "ctl1.vehicle.linearVelocity.selection", "ctl1.vehicle.angularVelocity.selection",
    ]
    assert all(field["resource"] == "vehicle" for field in fields)
    assert all(field["frameId"] == field["childFrameId"] == "body" for field in fields)
    assert FieldLayout([Field(**field) for field in fields]).size == width


def test_velocity_targets_require_an_explicit_nonempty_resource():
    with pytest.raises(TypeError):
        get_velocity_reference()
    with pytest.raises(ValueError):
        get_velocity_reference(resource="")
