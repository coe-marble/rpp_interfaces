"""Parameter helpers for use in workspace parameters.py files."""

from __future__ import annotations

from dataclasses import asdict

from .fields import Field

# Coordinate kinds and quantities from rpp_interfaces/common/constants.capnp.
_EUCLIDEAN = 1
_ANGLE = 2
_QUATERNION = 3
_LINEAR_POSITION = 1
_LINEAR_VELOCITY = 2
_ANGULAR_POSITION = 4
_ANGULAR_VELOCITY = 5
_FORCE = 7
_MOMENT = 8
_NORMALIZED = 9
_ORIENTATION = 13


def get_navigation_state(
    *,
    resource: str,
    dimensions: int = 2,
    frame_id: str = "map",
    child_frame_id: str = "base_link",
) -> dict:
    """Describe position, orientation, and body velocities in the plugin field order.

    Two dimensions use yaw; three dimensions use an xyzw quaternion.
    Each call returns a fresh dictionary, ready for the plugin's state parameter.
    """
    if dimensions not in (2, 3):
        raise ValueError("Navigation dimensions must be 2 or 3")
    if not resource:
        raise ValueError("The navigation resource must be nonempty")
    angular_size = 1 if dimensions == 2 else 3
    orientation_kind = _ANGLE if dimensions == 2 else _QUATERNION
    orientation_quantity = _ANGULAR_POSITION if dimensions == 2 else _ORIENTATION
    orientation_size = 1 if dimensions == 2 else 4
    fields = [
        Field(f"{resource}.position", _EUCLIDEAN, _LINEAR_POSITION, dimensions,
              frame_id, child_frame_id, resource),
        Field(f"{resource}.orientation", orientation_kind, orientation_quantity, orientation_size,
              frame_id, child_frame_id, resource),
        Field(f"{resource}.linearVelocity", _EUCLIDEAN, _LINEAR_VELOCITY, dimensions,
              child_frame_id, child_frame_id, resource),
        Field(f"{resource}.angularVelocity", _EUCLIDEAN, _ANGULAR_VELOCITY, angular_size,
              child_frame_id, child_frame_id, resource),
    ]
    return {"fields": [asdict(field) for field in fields]}


def get_navigation_reference(
    *,
    resource: str,
    dimensions: int = 2,
    frame_id: str = "map",
    child_frame_id: str = "base_link",
    inner_controller: str | None = None,
) -> dict:
    """Describe navigation targets for a reference generator's references parameter."""
    reference = get_navigation_state(
        dimensions=dimensions,
        frame_id=frame_id,
        child_frame_id=child_frame_id,
        resource=resource,
    )
    if inner_controller is not None:
        if not inner_controller:
            raise ValueError("The inner controller name must be nonempty")
        angular_size = 1 if dimensions == 2 else 3
        selections = [
            Field(f"{inner_controller}.{resource}.linearVelocity.selection",
                  _EUCLIDEAN, _NORMALIZED, dimensions, child_frame_id, child_frame_id, resource),
            Field(f"{inner_controller}.{resource}.angularVelocity.selection",
                  _EUCLIDEAN, _NORMALIZED, angular_size, child_frame_id, child_frame_id, resource),
        ]
        reference["fields"].extend(asdict(field) for field in selections)
    return reference


def get_wrench_reference(
    *,
    resource: str,
    dimensions: int = 2,
    frame_id: str = "base_link",
    inner_controller: str | None = None,
) -> dict:
    """Describe physical force/moment targets and optional cascade selectors."""
    if dimensions not in (2, 3) or not resource:
        raise ValueError("Wrench references require a resource and two or three dimensions")
    angular_size = 1 if dimensions == 2 else 3
    fields = [
        Field(f"{resource}.force", _EUCLIDEAN, _FORCE, dimensions, frame_id, frame_id, resource),
        Field(f"{resource}.moment", _EUCLIDEAN, _MOMENT, angular_size, frame_id, frame_id, resource),
    ]
    if inner_controller is not None:
        if not inner_controller:
            raise ValueError("The inner controller name must be nonempty")
        fields.extend([
            Field(f"{inner_controller}.{field.name}.selection", _EUCLIDEAN, _NORMALIZED,
                  field.size, frame_id, frame_id, resource)
            for field in fields
        ])
    return {"fields": [asdict(field) for field in fields]}


def get_pid_parameters(
    *,
    kp: float = 0.0,
    ki: float = 0.0,
    kd: float = 0.0,
    output_limit: float = 0.0,
    integral_limit: float = 0.0,
    anti_windup_gain: float = 1.0,
    derivative_filter_coefficient: float = 0.0,
) -> dict:
    """Build a PID parameter dictionary using the C++ PIDParameters defaults."""
    return {
        "kp": kp,
        "ki": ki,
        "kd": kd,
        "output_limit": output_limit,
        "integral_limit": integral_limit,
        "anti_windup_gain": anti_windup_gain,
        "derivative_filter_coefficient": derivative_filter_coefficient,
    }
