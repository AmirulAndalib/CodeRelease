/**
* @file TacticProvider.h
*
* Declaration of class TacticProvider.
* A collection of small tactical decisions and information gathering.
*
* @author <a href="mailto:ingmar.schwarz@tu-dortmund.de">Ingmar Schwarz</a>
*/

#pragma once

#include "Tools/Module/Module.h"
#include "Representations/Configuration/FieldDimensions.h"
#include "Representations/BehaviorControl/BallSymbols.h"
#include "Representations/BehaviorControl/GameSymbols.h"
#include "Representations/BehaviorControl/BehaviorConfiguration.h"
#include "Representations/BehaviorControl/TacticSymbols.h"
#include "Representations/Infrastructure/FrameInfo.h"
#include "Representations/Infrastructure/GameInfo.h"
#include "Representations/Infrastructure/RobotInfo.h"
#include "Representations/Infrastructure/TeamInfo.h"
#include "Representations/Infrastructure/TeammateData.h"
#include "Representations/BehaviorControl/BehaviorData.h"
#include "Representations/Modeling/RobotMap.h"
#include "Representations/Modeling/RobotPose.h"
#include "Tools/Settings.h"

MODULE(TacticProvider,
  REQUIRES(BallSymbols),
  REQUIRES(BehaviorConfiguration),
  REQUIRES(FrameInfo),
  REQUIRES(FieldDimensions),
  REQUIRES(GameInfo),
  REQUIRES(GameSymbols),
  REQUIRES(OwnTeamInfo),
  REQUIRES(OpponentTeamInfo),
  REQUIRES(RobotInfo),
  REQUIRES(RobotMap),
  REQUIRES(RobotPose),
  REQUIRES(RobotPoseAfterPreview),
  REQUIRES(TeammateData),
  PROVIDES(TacticSymbols),
  LOADS_PARAMETERS(,
    (unsigned)(5000) timeTillKeepRoleAssignmentInReady
  )
);

/**
* @class TacticProvider
* Collects information about the current tactical situation.s
*/
class TacticProvider : public TacticProviderBase
{
public:
  /** Constructor */
  TacticProvider()
  {
    lastKickoffWasOwn = false;
    lastOwnScore = 0;
    lastOpponentScore = 0;
  }

private:
  /** Updates some of the symbols */
  void update(TacticSymbols& tacticSymbols);

  /** vvv--- methods for tactical decisions ---vvv */
  void calcNumberOfActiveFieldPlayers(TacticSymbols& tacticSymbols);
  bool decideDefensiveBehavior();
  float decideActivity();
  void getBallDirection();
  void getBallSide();
  bool decideKickoffDirection(TacticSymbols& tacticSymbols);
  void decideFightForBall(TacticSymbols& tacticSymbols);
  void decideDefensiveCone(TacticSymbols& theTacticSymbols);

  enum class BallSide
  {
    front,
    center,
    back
  };

  enum class BallDirection
  {
    towardsEnemySide,
    towardsOwnSide
  };

  // vvv--- Attributes used to keep track of calculation results over several frames. ---vvv
  BallSide lastSide = BallSide::center;
  BallSide currentSide = BallSide::center;
  BallDirection lastDirection = BallDirection::towardsEnemySide;
  BallDirection currentDirection = BallDirection::towardsEnemySide;
  bool defensiveBehavior = false;

  float activity = 1.f;

  bool lastKickoffWasOwn;
  int lastOwnScore;
  int lastOpponentScore;
};
