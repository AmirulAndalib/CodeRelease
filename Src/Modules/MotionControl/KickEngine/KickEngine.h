/**
 * @file KickEngine.h
 * This file declares a module that creates the walking motions.
 * @author <A href="mailto:judy@tzi.de">Judith Müller</A>
 */

#pragma once

#include <optional>
#include <filesystem>
#include "KickEngineData.h"
#include "KickEngineParameters.h"
#include "Representations/Infrastructure/SensorData/JointSensorData.h"
#include "Representations/MotionControl/HeadJointRequest.h"
#include "Representations/MotionControl/KickEngineOutput.h"
#include "Representations/MotionControl/MotionSelection.h"
#include "Representations/MotionControl/SpecialActionsOutput.h"
#include "Representations/MotionControl/WalkingEngineOutput.h"
#include "Representations/Sensing/JoinedIMUData.h"
#include "Representations/Sensing/TorsoMatrix.h"
#include "Tools/Module/Module.h"
#include "Tools/Streams/InStreams.h"

MODULE(KickEngine,
  USES(RawJointRequest),
  REQUIRES(FrameInfo),
  REQUIRES(HeadJointRequest),
  REQUIRES(JointSensorData),
  REQUIRES(JointCalibration),
  REQUIRES(JoinedIMUData),
  REQUIRES(MassCalibration),
  REQUIRES(MotionRequest),
  REQUIRES(MotionSelection),
  REQUIRES(RobotDimensions),
  REQUIRES(RobotModel),
  REQUIRES(SpecialActionsOutput),
  REQUIRES(TorsoMatrix),
  REQUIRES(WalkingEngineOutput),
  PROVIDES(KickEngineOutput),
  LOADS_PARAMETERS(,
    ((JoinedIMUData) InertialDataSource)(JoinedIMUData::inertialSensorData) anglesource
  )
);

class KickEngine : public KickEngineBase
{
private:
  KickEngineData data;
  bool compensate = false;
  bool compensated = false;
  unsigned timeSinceLastPhase = 0;

  std::vector<KickEngineParameters> params;

public:
  KickEngine();

  void update(KickEngineOutput& kickEngineOutput);
};
