/**
* @file Modules/MotionControl/JointErrorCalc.h
* This file declares a module that calculates present joint error and join play error.
* @author <A href="mailto:mrunal.hatwar@tu-dortmund.de">Mrunal Hatwar</A>
*/
#pragma once
#include "Representations/Infrastructure/JointRequest.h"
#include "Representations/Infrastructure/SensorData/JointSensorData.h"
#include "Representations/Infrastructure/FrameInfo.h"
#include "Representations/MotionControl/JointError.h"
#include "Representations/MotionControl/WalkingEngineParams.h"
#include "Representations/MotionControl/MotionInfo.h"
#include "Representations/Sensing/GroundContactState.h"
#include "Representations/MotionControl/MotionRequest.h"
#include "Representations/MotionControl/Footpositions.h"
#include "Representations/MotionControl/FootSteps.h"
#include "Tools/Joints.h"
#include "Tools/Module/Module.h"
#include "Tools/Enum.h"

ENUM(JointPlayTrack,
  lhyp,
  lhr,
  lhp,
  lkp,
  lap,
  lar,
  rhyp,
  rhr,
  rhp,
  rkp,
  rap,
  rar
);

MODULE(JointErrorCalc,
  REQUIRES(FrameInfo),
  USES(RawJointRequest),
  REQUIRES(JointSensorData),
  USES(MotionInfo),
  REQUIRES(MotionRequest),
  REQUIRES(GroundContactState),
  REQUIRES(WalkingEngineParams),
  REQUIRES(SpeedRequest),
  PROVIDES(JointError),
  LOADS_PARAMETERS(,
      (Rangef) lowpassFilterFactor, /**< Low pass filter value */
      (float) interpolateLowpassFilterTime, /**< Start with high filter factor, but interpolate over 5 secs to the lower value. */
      (float) minWalkTime, /**< Start filtering after this much time, after robot started to walk. */
      (std::array<Angle, JointPlayTrack::numOfJointPlayTracks>) maxJointPlay, /**< Max joint play values, based on a good robot. */
      (std::array<float, JointPlayTrack::numOfJointPlayTracks>) maxJointPlayRatio, /**< Max joint play values, based on a good robot. */
      (Angle) jointPlayScalingWalkingSpeed, /**< When walking slower, a lower joint play is expected. */
      (float) minForwardSpeed, /**< scale expected joint play from this min speed. */
      (Angle) jointPlayScalingmin, /**< A robot below the minimum is good, a robot above bad. */
      (Angle) jointPlayScalingmax
  )
);

class JointErrorCalc : public JointErrorCalcBase
{
private:
  void update(JointError& jointError);
  void init(JointError& jointError);
  void jointPlayCalc(JointError& jointError);
  JointAngles jointAngleBuffer[5];
  bool initialized = false;
  int currentJointAngleID = 0;

  // Converts JointPlayTrack into the Joint enum
  Joints::Joint getJoint(JointPlayTrack joint);


  // Buffer for the joint request. Needed because of the motion delay, until a request is executed
  RingBufferWithSum<Angle, 4> bufferRequest[JointPlayTrack::numOfJointPlayTracks];

  // Filtered values over a long periode of time.
  Angle bufferValue[JointPlayTrack::numOfJointPlayTracks];

  // Filtered values over a long periode of time with a less strong low-pass filter parameter
  // Currently only used to analyse by hand. Shall be used in the future
  // to check for high changes within a few frames. Those indicate damaged gears/joints
  Angle bufferValueShortTerm[JointPlayTrack::numOfJointPlayTracks];

  // Temp buffer for the joint play, relative to a good robot
  Angle jointPlayList[JointPlayTrack::numOfJointPlayTracks];

  // Timestamp walking started
  unsigned int startWalkingTimestamp = 0;

  // Time spend walking
  float timeSpendWalking = 0.f;

  // Is robot currently walking?
  bool isWalking = false;


public:
};
