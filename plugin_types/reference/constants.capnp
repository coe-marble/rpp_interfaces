@0x80ede3eb4d8da632;

# API contract: ../../docs/component-boundaries.md

const generatorSetPoint :UInt8 = 1;
const generatorSetPath :UInt8 = 2;
const generatorSetTrajectory :UInt8 = 4;

const progressInvalid :UInt8 = 0;
const progressRunning :UInt8 = 1;
const progressReached :UInt8 = 2;
const progressFailed :UInt8 = 3;
