#include <ros/ros.h>
#include <sanchi_driver/SanchiRosDriver.h>
#include <sanchi_driver/utils.h>

using namespace std;
using namespace vwpp;


int main(int argc, char** argv)
{
    ros::init(argc, argv, "sanchi_driver_node");

    SanchiRosDriver sanchi_ros_driver;
    
    int publish_rate = 100;
    ros::NodeHandle nh("~");
    if (nh.hasParam("publish_rate"))
    {
        nh.getParam("publish_rate", publish_rate);
        ROS_INFO("%s use the publish data rate: %d Hz", ros::this_node::getName().c_str(), publish_rate);
    }
    else
    {
        ROS_WARN("%s use the default publish data rate %d Hz", ros::this_node::getName().c_str(), publish_rate);
    }

    // Publish data twice in a loop, so the real publish rate is loop_rate*2.
    ros::Rate loop_rate(publish_rate/2);
    while (ros::ok())
    {
        sanchi_ros_driver.publishData();
        loop_rate.sleep();
    }

    // Stop continous and close device
    ROS_WARN("Wait 0.1s"); 
    ros::Duration(0.1).sleep();

    return 0;
}
