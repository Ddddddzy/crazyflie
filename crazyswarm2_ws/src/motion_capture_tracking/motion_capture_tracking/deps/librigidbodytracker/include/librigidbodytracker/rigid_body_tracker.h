#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <Eigen/Geometry>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace librigidbodytracker {

using Point = pcl::PointXYZ;
using Cloud = pcl::PointCloud<Point>;
using MarkerConfiguration = Cloud::Ptr;

struct DynamicsConfiguration {
  float maxXVelocity;
  float maxYVelocity;
  float maxZVelocity;
  float maxRollRate;
  float maxPitchRate;
  float maxYawRate;
  float maxRoll;
  float maxPitch;
  float maxFitnessScore;
};

class RigidBody {
 public:
  RigidBody(
      size_t markerConfigurationIdx,
      size_t dynamicsConfigurationIdx,
      const Eigen::Affine3f& initialTransformation,
      const std::string& name);

  const Eigen::Affine3f& transformation() const;
  const Eigen::Affine3f& initialTransformation() const;
  bool lastTransformationValid() const;
  Eigen::Vector3f center() const { return m_lastTransformation.translation(); }
  Eigen::Vector3f initialCenter() const { return m_initialTransformation.translation(); }
  const std::string& name() const { return m_name; }
  bool orientationAvailable() const { return m_hasOrientation; }
  std::chrono::high_resolution_clock::time_point lastValidTime() const { return m_lastValidTransform; }

  size_t m_markerConfigurationIdx;
  size_t m_dynamicsConfigurationIdx;
  Eigen::Affine3f m_lastTransformation;
  Eigen::Vector3f m_velocity;
  bool m_hasOrientation;
  Eigen::Affine3f m_initialTransformation;
  std::chrono::high_resolution_clock::time_point m_lastValidTransform;
  bool m_lastTransformationValid;
  std::string m_name;
};

class RigidBodyTracker {
 public:
  RigidBodyTracker(
      const std::vector<DynamicsConfiguration>& dynamicsConfigurations,
      const std::vector<MarkerConfiguration>& markerConfigurations,
      const std::vector<RigidBody>& rigidBodies);

  void update(Cloud::Ptr pointCloud);
  void update(std::chrono::high_resolution_clock::time_point time,
      Cloud::Ptr pointCloud, std::string inputPath = "");

  const std::vector<RigidBody>& rigidBodies() const;

  void setLogWarningCallback(
      std::function<void(const std::string&)> logWarn);

 private:
  bool initializePose(Cloud::ConstPtr markers);
  void updatePose(std::chrono::high_resolution_clock::time_point stamp,
      Cloud::ConstPtr markers);
  bool initializePosition(std::chrono::high_resolution_clock::time_point stamp,
      Cloud::ConstPtr markers);
  void updatePosition(std::chrono::high_resolution_clock::time_point stamp,
      Cloud::ConstPtr markers);
  bool initializeHybrid(std::chrono::high_resolution_clock::time_point stamp,
      Cloud::ConstPtr markers);
  void updateHybrid(std::chrono::high_resolution_clock::time_point stamp,
      Cloud::ConstPtr markers);
  void logWarn(const std::string& msg);

  std::vector<MarkerConfiguration> m_markerConfigurations;
  std::vector<DynamicsConfiguration> m_dynamicsConfigurations;
  std::vector<RigidBody> m_rigidBodies;
  bool m_trackPositionOnly;
  enum { PositionMode, PoseMode, HybridMode } m_trackingMode;
  bool m_initialized;
  int m_init_attempts;
  std::function<void(const std::string&)> m_logWarn;
  std::string m_inputPath;
};

} // namespace librigidbodytracker
