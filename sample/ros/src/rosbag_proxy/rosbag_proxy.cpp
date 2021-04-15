#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "ros/ros.h"
#include <pcl_ros/point_cloud.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/ros/conversions.h>
#include "UDPServer.h"

#pragma pack(1)
struct CustomLidarPackage {
  char head[2];
  char version[2];
  uint32_t frame_id;
  uint64_t timestamp;
  uint32_t point_num;
  float points_buf[4 * 74];
  char tail[2];
};
#pragma pack()

uint64_t getCurrentTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<uint64_t>(tv.tv_sec * 1000000 + tv.tv_usec);
}

class rosbag_proxy {
  public:
    rosbag_proxy() 
    : nh("~"),
      points_topic(nh.param<std::string>("points_topic", "/points")),
      device_ip(nh.param<std::string>("device_ip", "192.168.1.100")),
      port(nh.param<int>("port", 2688))
    {
        std::cout << std::endl << "========================================" << std::endl;
        std::cout << "Transfer topic: " << points_topic << std::endl;
        std::cout << "Destination lidar plus IP: " << device_ip << std::endl;
        std::cout << "Destination Port: " << port << std::endl;
        udp_server.reset(new UDPServer(port + 10000));
        frame_id = 0;
        points_sub = nh.subscribe<sensor_msgs::PointCloud2>(points_topic, 10, &rosbag_proxy::points_callback, this);
    }

    void points_callback(const sensor_msgs::PointCloud2ConstPtr& data) {
        std::cout << "Get topic: " << points_topic <<  std::endl;
        pcl::PointCloud<pcl::PointXYZI>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZI>());
        pcl::fromROSMsg(*data, *cloud);

        CustomLidarPackage lidarPackage;
        lidarPackage.head[0] = ' ';
        lidarPackage.head[1] = 'e';
        lidarPackage.version[0] = '1';
        lidarPackage.version[1] = '0';
        lidarPackage.tail[0] = 'c';
        lidarPackage.tail[1] = 'n';
        lidarPackage.frame_id = frame_id;
        int point_num = 0;
        for (int index = 0; index < cloud->points.size(); index++) {
            lidarPackage.points_buf[point_num * 4 + 0] = cloud->points[index].x;
            lidarPackage.points_buf[point_num * 4 + 1] = cloud->points[index].y;
            lidarPackage.points_buf[point_num * 4 + 2] = cloud->points[index].z;
            lidarPackage.points_buf[point_num * 4 + 3] = cloud->points[index].intensity;
            point_num++;
            if (point_num == 74 || (index == (cloud->points.size() - 1))) {
                lidarPackage.timestamp = getCurrentTime();
                lidarPackage.point_num = point_num;
                point_num = 0;
                char buf[1206] = {0};
                memcpy(buf, &lidarPackage, sizeof(CustomLidarPackage));
                udp_server->UDPSendtoBuf(device_ip, port, buf, 1206);
            }
            if ((index % 1000) == 0) {
                usleep(100); //avoid buffer overrun
            }
        }
        frame_id++;
    }

  private:
    ros::NodeHandle nh;
    ros::Subscriber points_sub;
    std::string points_topic;
    std::string device_ip;
    int port;
    std::unique_ptr<UDPServer> udp_server;
    int frame_id;
};

int main(int argc, char **argv) {
    ros::init(argc, argv, "rosbag_proxy");
    rosbag_proxy node;
    ros::spin();
    return 0;
}