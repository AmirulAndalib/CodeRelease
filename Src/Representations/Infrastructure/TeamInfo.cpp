/**
 * @file TeamInfo.cpp
 * The file implements a struct that encapsulates the structure TeamInfo defined in
 * the file RoboCupGameControlData.h that is provided with the GameController.
 * @author <a href="mailto:Thomas.Roefer@dfki.de">Thomas Röfer</a>
 */

#include "TeamInfo.h"
#include "Tools/Debugging/DebugDrawings3D.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Settings.h"
#include <cstring>

/**
 * The struct is a helper to be able to stream the players.
 * The global RobotInfo cannot be used, because it has an additional attribute.
 */
struct PlayerInfo : public RoboCup::RobotInfo
{
};

/**
 * Write a player info to a stream.
 * @param stream The stream that is written to.
 * @param playerInfo The data that is written.
 * @return The stream.
 */
Out& operator<<(Out& stream, const PlayerInfo& playerInfo)
{
  STREAM_REGISTER_BEGIN_EXT(playerInfo);
  STREAM_EXT(stream, playerInfo.penalty);
  STREAM_EXT(stream, playerInfo.secsTillUnpenalised);
  STREAM_REGISTER_FINISH;
  return stream;
}

/**
 * Read a player info from a stream.
 * @param stream The stream that is read from.
 * @param playerInfo The data that is read.
 * @return The stream.
 */
In& operator>>(In& stream, PlayerInfo& playerInfo)
{
  STREAM_REGISTER_BEGIN_EXT(playerInfo);
  STREAM_EXT(stream, playerInfo.penalty);
  STREAM_EXT(stream, playerInfo.secsTillUnpenalised);
  STREAM_REGISTER_FINISH;
  return stream;
}

TeamInfo::TeamInfo()
{
  memset(static_cast<RoboCup::TeamInfo*>(this), 0, sizeof(RoboCup::TeamInfo));
}

void TeamInfo::serialize(In* in, Out* out)
{
  PlayerInfo(&players)[MAX_NUM_PLAYERS] = reinterpret_cast<PlayerInfo(&)[MAX_NUM_PLAYERS]>(this->players);

  STREAM_REGISTER_BEGIN;
  STREAM(teamNumber); // unique team number
  STREAM(teamColour); // TEAM_BLUE, TEAM_RED, TEAM_YELLOW, TEAM_BLACK
  STREAM(score); // team's score
  STREAM(messageBudget); // team's message budget
  STREAM(players); // the team's players
  STREAM(teamPort); // the team's UDP port
  STREAM_REGISTER_FINISH;
}

static void drawDigit(int digit, const Vector3f& pos, float size, int teamColor)
{
  static const Vector3f points[8] = {Vector3f(1, 0, 1), Vector3f(1, 0, 0), Vector3f(0, 0, 0), Vector3f(0, 0, 1), Vector3f(0, 0, 2), Vector3f(1, 0, 2), Vector3f(1, 0, 1), Vector3f(0, 0, 1)};
  static const unsigned char digits[10] = {0x3f, 0x0c, 0x76, 0x5e, 0x4d, 0x5b, 0x7b, 0x0e, 0x7f, 0x5f};
  const static ColorRGBA colors[] = {
      ColorRGBA::cyan,
      ColorRGBA::red,
      ColorRGBA::yellow,
      ColorRGBA::black,
      ColorRGBA::white,
      ColorRGBA::darkgreen,
      ColorRGBA::orange,
      ColorRGBA::purple,
      ColorRGBA::brown,
      ColorRGBA::gray,
  };

  digit = digits[std::abs(digit)];
  for (int i = 0; i < 7; ++i)
    if (digit & (1 << i))
    {
      Vector3f from = pos - points[i] * size;
      Vector3f to = pos - points[i + 1] * size;
      LINE3D("representation:TeamInfo", from.x(), from.y(), from.z(), to.x(), to.y(), to.z(), 2, colors[teamColor]);
    }
}

void TeamInfo::draw() const
{
  DECLARE_DEBUG_DRAWING3D("representation:TeamInfo", "field");
  {
    float x = teamNumber == 1 ? -1535.f : 1465.f;
    drawDigit(score / 10, Vector3f(x, 3500, 1000), 200, teamColour);
    drawDigit(score % 10, Vector3f(x + 270, 3500, 1000), 200, teamColour);
  };
}

OwnTeamInfo::OwnTeamInfo() {}

void OwnTeamInfo::draw() const
{
  //do base struct drawing first.
  TeamInfo::draw();

  DEBUG_DRAWING("representation:OwnTeamInfo", "drawingOnField")
  {
    DRAWTEXT("representation:OwnTeamInfo", -5000, -3800, 140, ColorRGBA::red, Settings::getName((Settings::TeamColor)teamColour));
  }
}

Streamable& OwnTeamInfo::operator=(const Streamable& other) noexcept
{
  return *this = dynamic_cast<const OwnTeamInfo&>(other);
}

OpponentTeamInfo::OpponentTeamInfo() {}

void OpponentTeamInfo::draw() const
{
  //do base struct drawing first.
  TeamInfo::draw();

  DEBUG_DRAWING("representation:OpponentTeamInfo", "drawingOnField")
  {
    DRAWTEXT("representation:OpponentTeamInfo", -5000, 3800, 140, ColorRGBA::red, Settings::getName((Settings::TeamColor)teamColour));
  }
}

Streamable& OpponentTeamInfo::operator=(const Streamable& other) noexcept
{
  return *this = dynamic_cast<const OpponentTeamInfo&>(other);
}
