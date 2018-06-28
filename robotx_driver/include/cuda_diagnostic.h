#ifndef CUDA_DIAGNOSTIC_H_INCLUDED
#define CUDA_DIAGNOSTIC_H_INCLUDED

//headers in ROS
#include <diagnostic_updater/diagnostic_updater.h>
#include <std_msgs/Bool.h>
#include <diagnostic_updater/publisher.h>
#include <ros/ros.h>

//headers in CUDA
#include <cuda_runtime.h>

/**
 * @brief cuda diagnostic class
 * it publish status of cuda devices
 */
class cuda_diagnostic
{
    public:
        /**
         * @brief parameters for cuda_diagnostic class
         * 
         */
        struct parameters
        {
            /**
             * @brief device id (use in diagnostic_updater)
             * 
             */
            std::string device_id;
            double update_frequency;
            parameters()
            {
                ros::param::param<std::string>(ros::this_node::getName()+"/device_id", device_id, "cuda_device");
                ros::param::param<double>(ros::this_node::getName()+"update_frequency", update_frequency, 1);
            }
        };
        cuda_diagnostic();
        ~cuda_diagnostic();
        void run();
    private:
        ros::NodeHandle nh_;
        diagnostic_updater::Updater updater_;
        const parameters params_;
        void update_(diagnostic_updater::DiagnosticStatusWrapper &stat);
};
#endif  //CUDA_DIAGNOSTIC_H_INCLUDED