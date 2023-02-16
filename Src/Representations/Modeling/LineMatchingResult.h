/**
* @file LineMatchingResult.h
* Declaration of a class that represents the line matching result.
* @author <a href="mailto:stefan.tasse@tu-dortmund.de">Stefan Tasse</a>
*/

#pragma once

#include "Tools/Streams/AutoStreamable.h"
#include "Tools/Math/Pose2f.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Debugging/DebugDrawings.h"
#include <vector>
#include <algorithm>

/**
* @class LineMatchingResult
* A class that represents the line matching result.
*/
STREAMABLE(LineMatchingResult,
public:
  enum
  {
    maxNumberOfLineObservations = 8 // to have a fixed size for the correspondence array
  };

  STREAMABLE(FieldLine,
    FieldLine(){}
    FieldLine(const Vector2d& start_, const Vector2d& end_, const double cameraHeight)
    {
      this->start = start_;
      this->end = end_;
      this->cameraHeight = cameraHeight;
    }
    ,
    (Vector2d) start,
    (Vector2d) end,
    (double)(0.0) cameraHeight
  );

  STREAMABLE(PoseHypothesis,
    PoseHypothesis() { memset(lineCorrespondences, 0, sizeof(lineCorrespondences)); }

    void setLineCorrespondences(const int otherLineCorrespondences[])
    {
      for (int i=0; i<maxNumberOfLineObservations; i++)
      {
        lineCorrespondences[i] = otherLineCorrespondences[i];
      }
    }
    ,
    (Pose2f) pose,
    (int[maxNumberOfLineObservations]) lineCorrespondences
  );

  STREAMABLE(PoseHypothesisInterval,
    PoseHypothesisInterval() { memset(lineCorrespondences, 0, sizeof(lineCorrespondences)); }
    ,
    (Pose2f) start,
    (Pose2f) end,
    (int[maxNumberOfLineObservations]) lineCorrespondences
  );

private:
  std::vector<FieldLine> observationsSphericalCoords;

public:
  /**
  * Default constructor.
  */
  LineMatchingResult() { reset(); }

  /** Reset the path */
  void reset()
  {
    observations.clear();
    observationsSphericalCoords.clear();
    poseHypothesis.clear();
    poseHypothesisIntervals.clear();
    onlyObservedOneFieldLine = false;
  }

private:

  inline Vector2d getSphericalCoordinatesForPointObservation(const Vector2d & pointInRelativeCoords, const double &cameraHeight) const
  {
    Vector2d sphericalObservation;
    sphericalObservation.x() = atan2(cameraHeight, pointInRelativeCoords.norm()); // vertical angle
    sphericalObservation.y() = pointInRelativeCoords.angle(); // horizontal angle
    return sphericalObservation;
  }

  inline Vector2d getSphericalCoordinatesForPointObservation(const Pose2f & pose, const Vector2d & pointInAbsoluteCoords, const double &cameraHeight) const
  {
    Vector2d pointInRelativeCoords = pointInAbsoluteCoords - pose.translation.cast<double>();
    pointInRelativeCoords.rotate(-pose.rotation);
    return getSphericalCoordinatesForPointObservation(pointInRelativeCoords, cameraHeight);
  }


  inline FieldLine getSphericalCoordinatesForPointObservation(const FieldLine & fieldLineInRelativeCoords) const
  {
    FieldLine sphericalObservation;
    sphericalObservation.start = getSphericalCoordinatesForPointObservation(fieldLineInRelativeCoords.start, fieldLineInRelativeCoords.cameraHeight);
    sphericalObservation.end   = getSphericalCoordinatesForPointObservation(fieldLineInRelativeCoords.end, fieldLineInRelativeCoords.cameraHeight);
    sphericalObservation.cameraHeight = fieldLineInRelativeCoords.cameraHeight;
    return sphericalObservation;
  }

  inline FieldLine getSphericalCoordinatesForPointObservation(const Pose2f & pose, const FieldLine & fieldLineInAbsoluteCoords) const
  {
    FieldLine sphericalObservation;
    sphericalObservation.start = getSphericalCoordinatesForPointObservation(pose, fieldLineInAbsoluteCoords.start, fieldLineInAbsoluteCoords.cameraHeight);
    sphericalObservation.end   = getSphericalCoordinatesForPointObservation(pose, fieldLineInAbsoluteCoords.end, fieldLineInAbsoluteCoords.cameraHeight);
    sphericalObservation.cameraHeight = fieldLineInAbsoluteCoords.cameraHeight;
    return sphericalObservation;
  }

  double calculateMeasurementLikelihoodSphericalCoordinates(const PoseHypothesis& poseHypothesis, const Matrix2d measurementCovariance_inv) const;

public:

  void calculateObservationsSphericalCoords(); // should be called by the LineMatcher after filling with the observations

  bool getCorrespondencesForLocalizationHypothesis(
    const Pose2f & localizationHypothesis, 
    const Matrix3d & poseCovariance, 
    double likelihoodThreshold, 
    const Matrix2d & sphericalPointMeasurementCovariance_inv, 
    std::vector<FieldLine> & correspondencesForObservations,
    bool displayWarning=false,
    bool requestedByLocalization=true) const;

  Vector2d projectPointToFieldLine(const Pose2f & pose, const Vector2d & pointInRelativeCoords, const FieldLine & line) const;
  Vector2d projectPointToFieldLine(const Vector2d & pointInAbsoluteCoords, const FieldLine & line) const;

  bool containsMatches() const
  {
    return poseHypothesis.size() > 0 || poseHypothesisIntervals.size() > 0;
  }
  bool containsUniqueMatches() const
  {
    return poseHypothesis.size() > 0;
  }
  bool containsNonUniqueMatches() const
  {
    return poseHypothesisIntervals.size() > 0;
  }

  void drawCorrespondences(const Pose2f & pose) const;
  void drawRequestedCorrespondences(const Pose2f & pose, const Matrix3d & cov, double likelihoodThreshold, const Matrix2d & measurementCov) const;

  /**
  * The method draws the line matching result.
  */
  void draw() const;
  ,

  (std::vector<FieldLine>) fieldLines,
  (std::vector<FieldLine>) observations,
  (std::vector<PoseHypothesis>) poseHypothesis, /**< Possible poses in absolute field coordinates (for unique poses, i.e. at least one crossing). */
  (std::vector<PoseHypothesisInterval>) poseHypothesisIntervals, /**< Possible poses in absolute field coordinates (for pose intervals, i.e. only two parallel lines). */
  (bool)(false) onlyObservedOneFieldLine
);
