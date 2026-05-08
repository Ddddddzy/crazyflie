/**
*
*  \author     Paul Bovbel <pbovbel@clearpathrobotics.com>
*  \copyright  Copyright (c) 2015, Clearpath Robotics, Inc.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Clearpath Robotics, Inc. nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL CLEARPATH ROBOTICS, INC. BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Please send comments, questions, or patches to code@clearpathrobotics.com
*
*/

#include "vrpn_client_ros/vrpn_client_ros.h"

#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"
#include "tf2_ros/transform_broadcaster.h"

#include "rclcpp/logging.hpp"

#include <vector>
#include <unordered_set>
#include <algorithm>
#include <chrono>

namespace
{
  std::unordered_set<std::string> name_blacklist_({"VRPN Control"});
}


namespace vrpn_client_ros
{
  using namespace std::chrono_literals;

  /**
   * check Ros Names as defined here: http://wiki.ros.org/Names
   */
  bool isInvalidFirstCharInName(const char c)
  {
    return ! ( isalpha(c) || c == '/' || c == '~' );
  }

  bool isInvalidSubsequentCharInName(const char c)
  {
    return ! ( isalnum(c) || c == '/' || c == '_' );
  }

  VrpnTrackerRos::VrpnTrackerRos(std::string tracker_name, ConnectionPtr connection, rclcpp::Node::SharedPtr nh)
  {
    tracker_remote_ = std::make_shared<vrpn_Tracker_Remote>(tracker_name.c_str(), connection.get());

    std::string clean_name = tracker_name;

    if (clean_name.size() > 0)
    {
      int start_subsequent = 1;
      if (isInvalidFirstCharInName(clean_name[0])) 
      {
        clean_name = clean_name.substr(1);
        start_subsequent = 0;
      }

      clean_name.erase( std::remove_if( clean_name.begin() + start_subsequent, clean_name.end(), isInvalidSubsequentCharInName ), clean_name.end() );
    }

    init(clean_name, nh, false, nullptr);
  }

  VrpnTrackerRos::VrpnTrackerRos(std::string tracker_name, std::string host, rclcpp::Node::SharedPtr nh)
  {
    std::string tracker_address;
    tracker_address = tracker_name + "@" + host;
    tracker_remote_ = std::make_shared<vrpn_Tracker_Remote>(tracker_address.c_str());
    init(tracker_name, nh, true, nullptr);
  }

  void VrpnTrackerRos::init(std::string tracker_name, rclcpp::Node::SharedPtr nh, bool create_mainloop_timer, VrpnClientRos *client)
  {
    RCLCPP_INFO_STREAM(nh->get_logger(), "Creating new tracker " << tracker_name);

    tracker_remote_->register_change_handler(this, &VrpnTrackerRos::handle_pose);
    tracker_remote_->register_change_handler(this, &VrpnTrackerRos::handle_twist);
    tracker_remote_->register_change_handler(this, &VrpnTrackerRos::handle_accel);
    tracker_remote_->shutup = true;

    rclcpp::expand_topic_or_service_name(
            tracker_name,
            nh->get_name(),
            nh->get_namespace()
    );  // will throw an error if invalid

    this->tracker_name = tracker_name;
    client_ = client;

    output_nh_ = nh->create_sub_node(tracker_name);

    std::string frame_id;
    nh->get_parameter("frame_id", frame_id);
    nh->get_parameter("use_server_time", use_server_time_);
    nh->get_parameter("broadcast_tf", broadcast_tf_);

    pose_msg_.header.frame_id = frame_id;
    // pose_msg_.header.frame_id = twist_msg_.header.frame_id = accel_msg_.header.frame_id = transform_stamped_.header.frame_id = frame_id;

    if (create_mainloop_timer)
    {
      double update_frequency;
      nh->get_parameter("update_frequency", update_frequency);
      mainloop_timer = nh->create_wall_timer(1s/update_frequency, std::bind(&VrpnTrackerRos::mainloop, this));
    }
  }

  VrpnTrackerRos::~VrpnTrackerRos()
  {
    RCLCPP_INFO_STREAM(output_nh_->get_logger(), "Destroying tracker " << tracker_name);
    tracker_remote_->unregister_change_handler(this, &VrpnTrackerRos::handle_pose);
    tracker_remote_->unregister_change_handler(this, &VrpnTrackerRos::handle_twist);
    tracker_remote_->unregister_change_handler(this, &VrpnTrackerRos::handle_accel);
  }

  void VrpnTrackerRos::mainloop()
  {
    tracker_remote_->mainloop();
  }

  void VrpnTrackerRos::setClient(VrpnClientRos *client)
  {
    client_ = client;
  }

  void VRPN_CALLBACK VrpnTrackerRos::handle_pose(void *userData, const vrpn_TRACKERCB tracker_pose)
  {
    VrpnTrackerRos *tracker = static_cast<VrpnTrackerRos *>(userData);
    rclcpp::Node::SharedPtr nh = tracker->output_nh_;

    if (tracker->use_server_time_)
    {
      tracker->pose_msg_.header.stamp.sec = tracker_pose.msg_time.tv_sec;
      tracker->pose_msg_.header.stamp.nanosec = tracker_pose.msg_time.tv_usec * 1000;
    }
    else
    {
      tracker->pose_msg_.header.stamp = nh->now();
    }

    tracker->pose_msg_.pose.position.x = tracker_pose.pos[0];
    tracker->pose_msg_.pose.position.y = tracker_pose.pos[1];
    tracker->pose_msg_.pose.position.z = tracker_pose.pos[2];

    tracker->pose_msg_.pose.orientation.x = tracker_pose.quat[0];
    tracker->pose_msg_.pose.orientation.y = tracker_pose.quat[1];
    tracker->pose_msg_.pose.orientation.z = tracker_pose.quat[2];
    tracker->pose_msg_.pose.orientation.w = tracker_pose.quat[3];

    if (tracker->client_ != nullptr)
    {
      tracker->client_->handlePoseUpdate(
        tracker->tracker_name,
        tracker->pose_msg_.pose,
        tracker->pose_msg_.header.stamp);
    }
  
    // if (tracker->broadcast_tf_)
    // {
    //   static tf2_ros::TransformBroadcaster tf_broadcaster;

    //   if (tracker->use_server_time_)
    //   {
    //     tracker->transform_stamped_.header.stamp.sec = tracker_pose.msg_time.tv_sec;
    //     tracker->transform_stamped_.header.stamp.nsec = tracker_pose.msg_time.tv_usec * 1000;
    //   }
    //   else
    //   {
    //     tracker->transform_stamped_.header.stamp = ros::Time::now();
    //   }

    //   if (tracker->process_sensor_id_)
    //   {
    //     tracker->transform_stamped_.child_frame_id = tracker->tracker_name + "/" + std::to_string(tracker_pose.sensor);
    //   }
    //   else
    //   {
    //     tracker->transform_stamped_.child_frame_id = tracker->tracker_name;
    //   }

    //   tracker->transform_stamped_.transform.translation.x = tracker_pose.pos[0];
    //   tracker->transform_stamped_.transform.translation.y = tracker_pose.pos[1];
    //   tracker->transform_stamped_.transform.translation.z = tracker_pose.pos[2];

    //   tracker->transform_stamped_.transform.rotation.x = tracker_pose.quat[0];
    //   tracker->transform_stamped_.transform.rotation.y = tracker_pose.quat[1];
    //   tracker->transform_stamped_.transform.rotation.z = tracker_pose.quat[2];
    //   tracker->transform_stamped_.transform.rotation.w = tracker_pose.quat[3];

    //   tf_broadcaster.sendTransform(tracker->transform_stamped_);
    // }
  }

  void VRPN_CALLBACK VrpnTrackerRos::handle_twist(void *userData, const vrpn_TRACKERVELCB tracker_twist)
  {
    VrpnTrackerRos *tracker = static_cast<VrpnTrackerRos *>(userData);
    rclcpp::Node::SharedPtr nh = tracker->output_nh_;

    if (tracker->use_server_time_)
    {
      tracker->twist_msg_.header.stamp.sec = tracker_twist.msg_time.tv_sec;
      tracker->twist_msg_.header.stamp.nanosec = tracker_twist.msg_time.tv_usec * 1000;
    }
    else
    {
      tracker->twist_msg_.header.stamp = nh->now();;
    }

    tracker->twist_msg_.twist.linear.x = tracker_twist.vel[0];
    tracker->twist_msg_.twist.linear.y = tracker_twist.vel[1];
    tracker->twist_msg_.twist.linear.z = tracker_twist.vel[2];

    double roll, pitch, yaw;
    tf2::Matrix3x3 rot_mat(
        tf2::Quaternion(tracker_twist.vel_quat[0], tracker_twist.vel_quat[1], tracker_twist.vel_quat[2],
                        tracker_twist.vel_quat[3]));
    rot_mat.getRPY(roll, pitch, yaw);

    tracker->twist_msg_.twist.angular.x = roll;
    tracker->twist_msg_.twist.angular.y = pitch;
    tracker->twist_msg_.twist.angular.z = yaw;

    if (tracker->client_ != nullptr)
    {
      tracker->client_->handleTwistUpdate(
        tracker->tracker_name,
        tracker->twist_msg_.twist,
        tracker->twist_msg_.header.stamp);
    }
  }

  void VRPN_CALLBACK VrpnTrackerRos::handle_accel(void *userData, const vrpn_TRACKERACCCB tracker_accel)
  {
    VrpnTrackerRos *tracker = static_cast<VrpnTrackerRos *>(userData);
    rclcpp::Node::SharedPtr nh = tracker->output_nh_;

    if (tracker->use_server_time_)
    {
      tracker->accel_msg_.header.stamp.sec = tracker_accel.msg_time.tv_sec;
      tracker->accel_msg_.header.stamp.nanosec = tracker_accel.msg_time.tv_usec * 1000;
    }
    else
    {
      tracker->accel_msg_.header.stamp = nh->now();;
    }

    tracker->accel_msg_.accel.linear.x = tracker_accel.acc[0];
    tracker->accel_msg_.accel.linear.y = tracker_accel.acc[1];
    tracker->accel_msg_.accel.linear.z = tracker_accel.acc[2];

    double roll, pitch, yaw;
    tf2::Matrix3x3 rot_mat(
        tf2::Quaternion(tracker_accel.acc_quat[0], tracker_accel.acc_quat[1], tracker_accel.acc_quat[2],
                        tracker_accel.acc_quat[3]));
    rot_mat.getRPY(roll, pitch, yaw);

    tracker->accel_msg_.accel.angular.x = roll;
    tracker->accel_msg_.accel.angular.y = pitch;
    tracker->accel_msg_.accel.angular.z = yaw;

    if (tracker->client_ != nullptr)
    {
      tracker->client_->handleAccelUpdate(
        tracker->tracker_name,
        tracker->accel_msg_.accel,
        tracker->accel_msg_.header.stamp);
    }
  }

  VrpnClientRos::VrpnClientRos(rclcpp::Node::SharedPtr nh, rclcpp::Node::SharedPtr private_nh)
  {
    output_nh_ = private_nh;

    nh->declare_parameter("server", "localhost");
    nh->declare_parameter("port", 3883);
    nh->declare_parameter("update_frequency", 100.0);
    nh->declare_parameter("frame_id", "world");
    nh->declare_parameter("use_server_time", false);
    nh->declare_parameter("poses_qos_deadline", 100.0);
    //nh->declare_parameter("broadcast_tf", true);
    nh->declare_parameter("refresh_tracker_frequency", 1.0);

    std::vector<std::string> param_tracker_names;
    nh->declare_parameter("trackers", param_tracker_names);


    host_ = getHostStringFromParams(private_nh);
    private_nh->get_parameter("frame_id", frame_id_);
    private_nh->get_parameter("poses_qos_deadline", poses_qos_deadline_);

    RCLCPP_INFO_STREAM(output_nh_->get_logger(), "Connecting to VRPN server at " << host_);
    connection_ = std::shared_ptr<vrpn_Connection>(vrpn_get_connection_by_name(host_.c_str()));
    RCLCPP_INFO(output_nh_->get_logger(), "Connection established");

    rclcpp::SensorDataQoS sensor_data_qos;
    sensor_data_qos.keep_last(1);
    sensor_data_qos.deadline(rclcpp::Duration(0, static_cast<int32_t>(1e9 / poses_qos_deadline_)));

    poses_pub_ = nh->create_publisher<motion_capture_tracking_interfaces::msg::NamedPoseArray>("poses", sensor_data_qos);
    twists_pub_ = nh->create_publisher<motion_capture_tracking_interfaces::msg::NamedTwistArray>("velocity", sensor_data_qos);
    accels_pub_ = nh->create_publisher<motion_capture_tracking_interfaces::msg::NamedAccelArray>("accel", sensor_data_qos);

    double update_frequency;
    private_nh->get_parameter("update_frequency", update_frequency);

    mainloop_timer = nh->create_wall_timer(1s/update_frequency, std::bind(&VrpnClientRos::mainloop, this));

    double refresh_tracker_frequency;
    private_nh->get_parameter("refresh_tracker_frequency", refresh_tracker_frequency);

    if (refresh_tracker_frequency > 0.0)
    {
      refresh_tracker_timer = nh->create_wall_timer(1s/refresh_tracker_frequency, 
              std::bind(&VrpnClientRos::updateTrackers, this));
    }

    std::vector<std::string> param_tracker_names_;
    if (private_nh->get_parameter("trackers", param_tracker_names_))
    {
      for (std::vector<std::string>::iterator it = param_tracker_names_.begin();
           it != param_tracker_names_.end(); ++it)
      {
        trackers_.insert(std::make_pair(*it, std::make_shared<VrpnTrackerRos>(*it, connection_, output_nh_)));
        trackers_.at(*it)->setClient(this);
      }
    }
  }

  std::string VrpnClientRos::getHostStringFromParams(rclcpp::Node::SharedPtr host_nh)
  {
    std::stringstream host_stream;
    std::string server;
    int port;


    host_nh->get_parameter("server", server);
    host_stream << server;

    if (host_nh->get_parameter("port", port))
    {
      host_stream << ":" << port;
    }
    return host_stream.str();
  }

  void VrpnClientRos::mainloop()
  {
    connection_->mainloop();
    if (!connection_->doing_okay())
    {
      RCLCPP_WARN(output_nh_->get_logger(), "VRPN connection is not 'doing okay'");
    }
    for (TrackerMap::iterator it = trackers_.begin(); it != trackers_.end(); ++it)
    {
      it->second->mainloop();
    }
  }

  void VrpnClientRos::updateTrackers()
  {
    int i = 0;
    while (connection_->sender_name(i) != NULL)
    {
      if (trackers_.count(connection_->sender_name(i)) == 0 && name_blacklist_.count(connection_->sender_name(i)) == 0)
      {
        RCLCPP_INFO_STREAM(output_nh_->get_logger(), "Found new sender: " << connection_->sender_name(i));
        trackers_.insert(std::make_pair(connection_->sender_name(i),
                                        std::make_shared<VrpnTrackerRos>(connection_->sender_name(i), connection_,
                                                                           output_nh_)));
        trackers_.at(connection_->sender_name(i))->setClient(this);
      }
      i++;
    }
  }

  void VrpnClientRos::handlePoseUpdate(
    const std::string &tracker_name,
    const geometry_msgs::msg::Pose &pose,
    const builtin_interfaces::msg::Time &stamp)
  {
    latest_poses_[tracker_name] = pose;

    motion_capture_tracking_interfaces::msg::NamedPoseArray msg;
    msg.header.frame_id = frame_id_;
    msg.header.stamp = stamp;
    msg.poses.reserve(latest_poses_.size());

    for (const auto &entry : latest_poses_)
    {
      motion_capture_tracking_interfaces::msg::NamedPose named_pose;
      named_pose.name = entry.first;
      named_pose.pose = entry.second;
      msg.poses.push_back(named_pose);
    }

    poses_pub_->publish(msg);
  }

  void VrpnClientRos::handleTwistUpdate(
    const std::string &tracker_name,
    const geometry_msgs::msg::Twist &twist,
    const builtin_interfaces::msg::Time &stamp)
  {
    latest_twists_[tracker_name] = twist;

    motion_capture_tracking_interfaces::msg::NamedTwistArray msg;
    msg.header.frame_id = frame_id_;
    msg.header.stamp = stamp;
    msg.twists.reserve(latest_twists_.size());

    for (const auto &entry : latest_twists_)
    {
      motion_capture_tracking_interfaces::msg::NamedTwist named_twist;
      named_twist.name = entry.first;
      named_twist.twist = entry.second;
      msg.twists.push_back(named_twist);
    }

    twists_pub_->publish(msg);
  }

  void VrpnClientRos::handleAccelUpdate(
    const std::string &tracker_name,
    const geometry_msgs::msg::Accel &accel,
    const builtin_interfaces::msg::Time &stamp)
  {
    latest_accels_[tracker_name] = accel;

    motion_capture_tracking_interfaces::msg::NamedAccelArray msg;
    msg.header.frame_id = frame_id_;
    msg.header.stamp = stamp;
    msg.accels.reserve(latest_accels_.size());

    for (const auto &entry : latest_accels_)
    {
      motion_capture_tracking_interfaces::msg::NamedAccel named_accel;
      named_accel.name = entry.first;
      named_accel.accel = entry.second;
      msg.accels.push_back(named_accel);
    }

    accels_pub_->publish(msg);
  }
}  // namespace vrpn_client_ros
