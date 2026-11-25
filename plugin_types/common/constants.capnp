@0x8871545d9f17e9df;

# API contract: ../../docs/shared-bind-and-runtime.md

const contractVersion :UInt8 = 1;

const codeInvalid :UInt8 = 0;
const codeOk :UInt8 = 1;
const codeLimited :UInt8 = 2;
const codeNotReady :UInt8 = 3;
const codeRejected :UInt8 = 4;
const codeIncompatible :UInt8 = 5;
const codeInvalidInput :UInt8 = 6;
const codeStaleInput :UInt8 = 7;
const codeInfeasible :UInt8 = 8;
const codeBudgetExceeded :UInt8 = 9;
const codeHardwareFault :UInt8 = 10;
const codeUnsupported :UInt8 = 11;

const coordinateInvalid :UInt8 = 0;
const coordinateEuclidean :UInt8 = 1; # Stored/tangent widths: size/size, size > 0.
const coordinateContinuousAngle :UInt8 = 2; # Stored/tangent widths: size/size; zero size infers 1/1.
const coordinateQuaternion :UInt8 = 3; # Stored/tangent widths: 4/3, unit quaternion xyzw.

# Quantity fixes the storage unit. Drivers convert to these units before publishing.
const quantityInvalid :UInt8 = 0;
const quantityLinearPosition :UInt8 = 1; # m
const quantityLinearVelocity :UInt8 = 2; # m/s
const quantityLinearAcceleration :UInt8 = 3; # m/s^2
const quantityAngularPosition :UInt8 = 4; # rad
const quantityAngularVelocity :UInt8 = 5; # rad/s
const quantityAngularAcceleration :UInt8 = 6; # rad/s^2
const quantityForce :UInt8 = 7; # N
const quantityMoment :UInt8 = 8; # N m
const quantityNormalized :UInt8 = 9; # Dimensionless.
const quantityVoltage :UInt8 = 10; # V
const quantityCurrent :UInt8 = 11; # A
const quantityPressure :UInt8 = 12; # Pa
const quantityOrientation :UInt8 = 13; # Dimensionless quaternion coefficients; tangent increments use rad.
const quantityTemperature :UInt8 = 14; # K
