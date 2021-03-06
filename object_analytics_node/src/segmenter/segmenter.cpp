/*
 * Copyright (c) 2018 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define PCL_NO_PRECOMPILE
#include <memory>
#include <vector>
#include <pcl_conversions/pcl_conversions.h>
#include <object_analytics_msgs/msg/objects_in_boxes3_d.hpp>
#include <object_analytics_msgs/msg/object_in_box3_d.hpp>
#include "object_analytics_node/segmenter/organized_multi_plane_segmenter.hpp"
#include "object_analytics_node/segmenter/segmenter.hpp"

namespace object_analytics_node
{
namespace segmenter
{
using pcl::fromROSMsg;
using pcl::Label;
using pcl::Normal;
using pcl::PointIndices;
using pcl::IndicesPtr;
using object_analytics_node::model::Object3D;

Segmenter::Segmenter(std::unique_ptr<AlgorithmProvider> provider) : provider_(std::move(provider))
{
}

void Segmenter::segment(const sensor_msgs::msg::PointCloud2::ConstSharedPtr& points, ObjectsInBoxes3D::SharedPtr& msg)
{
  PointCloudT::Ptr pointcloud(new PointCloudT);
  getPclPointCloud(points, *pointcloud);

  std::vector<Object3D> objects;
  doSegment(pointcloud, objects);

  composeResult(objects, msg);
}

void Segmenter::getPclPointCloud(const sensor_msgs::msg::PointCloud2::ConstSharedPtr& points, PointCloudT& pcl_cloud)
{
  fromROSMsg<PointT>(*points, pcl_cloud);
}

void Segmenter::doSegment(const PointCloudT::ConstPtr& cloud, std::vector<Object3D>& objects)
{
  std::vector<PointIndices> cluster_indices;
  PointCloudT::Ptr cloud_segment(new PointCloudT);
  std::shared_ptr<Algorithm> seg = provider_->get();
  seg->segment(cloud, cloud_segment, cluster_indices);

  for (auto& indices : cluster_indices)
  {
    try
    {
      Object3D object3d(cloud_segment, indices.indices);
      objects.push_back(object3d);
    }
    catch (std::exception& e)
    {
      // ROS_ERROR_STREAM(e.what());
    }
  }

  // ROS_DEBUG_STREAM("get " << objects.size() << " objects from segmentation");
}

void Segmenter::composeResult(const std::vector<Object3D>& objects, ObjectsInBoxes3D::SharedPtr& msg)
{
  for (auto& obj : objects)
  {
    object_analytics_msgs::msg::ObjectInBox3D oib3;
    oib3.min = obj.getMin();
    oib3.max = obj.getMax();
    oib3.roi = obj.getRoi();
    msg->objects_in_boxes.push_back(oib3);
  }

  // ROS_DEBUG_STREAM("segmenter publish message with " << objects.size() << " objects");
}
}  // namespace segmenter
}  // namespace object_analytics_node
