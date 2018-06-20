#include <robotx_localization.h>

robotx_localization::robotx_localization() : params_()
{
    Eigen::VectorXd init_value = Eigen::VectorXd::Ones(3);
    init_value = init_value * 0.5;
    std::vector<bool> is_circular(3);
    is_circular[0] = false;
    is_circular[1] = false;
    is_circular[2] = true;
    pfilter_ptr_ = new particle_filter(3,params_.num_particles,init_value,is_circular);
    fix_recieved_ = false;
    twist_received_ = false;
    fix_sub_ = nh_.subscribe(params_.fix_topic, 1, &robotx_localization::fix_callback_, this);
    twist_sub_ = nh_.subscribe(params_.twist_topic, 1, &robotx_localization::twist_callback_, this);
    thread_update_frame_ = boost::thread(boost::bind(&robotx_localization::update_frame_, this));
}

robotx_localization::~robotx_localization()
{
    thread_update_frame_.join();
}

void robotx_localization::update_frame_()
{
    ros::Rate rate(params_.publish_rate);
    while(fix_recieved_ == false)
    {
        rate.sleep();
    }
    while(ros::ok())
    {
        std::lock(fix_mutex_,twist_mutex_);
        //critical section start
        pfilter_ptr_->resample(params_.ess_threshold);
        Eigen::VectorXd control_input(3);
        Eigen::VectorXd state = pfilter_ptr_->get_state();
        Eigen::VectorXd position(3);
        position(0) = params_.min_x + state(0)*(params_.max_x-params_.min_x);
        position(1) = params_.min_y + state(1)*(params_.max_y-params_.min_y);
        position(2) = 2*M_PI*state(2);
        control_input(0) = std::cos(position(2)) - std::sin(position(2));
        control_input(1) = std::sin(position(2)) + std::cos(position(2));
        control_input(2) = position(2);
        control_input(0) = control_input(0)/(params_.max_x - params_.min_x);
        control_input(1) = control_input(1)/(params_.max_y - params_.min_y);
        control_input(2) = control_input(2)/(2*M_PI);
        pfilter_ptr_->add_system_noise(control_input,0.1);
        Eigen::MatrixXd states = pfilter_ptr_->get_states();
        double measurement_x = (last_fix_message_.longitude-init_measurement_.longitude)*111263.283/(params_.max_x-params_.min_x);
        double measurement_y = (last_fix_message_.latitude-init_measurement_.latitude)*6378150*std::cos(last_fix_message_.longitude/180*M_PI)*2*M_PI/(360*60*60)/(params_.max_y-params_.min_y);
        Eigen::VectorXd weights(params_.num_particles);
        for(int i=0; i<params_.num_particles; i++)
        {
            double error = std::sqrt(std::pow(states(0,i)-measurement_x,2) + std::pow(states(1,i)-measurement_y,2));
            // avoid zero dicision
            if(error == 0)
                error = 0.000000000000001;
            weights(i) = 1/error;
        }
        pfilter_ptr_->set_weights(weights);
        Eigen::VectorXd predicted_pos = pfilter_ptr_->get_state();
        //ROS_ERROR_STREAM(predicted_pos(0) << "," << predicted_pos(1) << "," << predicted_pos(2));
        //critical section end
        fix_mutex_.unlock();
        twist_mutex_.unlock();
        rate.sleep();
    }
}

void robotx_localization::fix_callback_(sensor_msgs::NavSatFix msg)
{
    std::lock_guard<std::mutex> lock(fix_mutex_);
    if(fix_recieved_ == false)
    {
        init_measurement_ = msg;
    }
    last_fix_message_ = msg;
    fix_recieved_ = true;
}

void robotx_localization::twist_callback_(geometry_msgs::Twist msg)
{
    std::lock_guard<std::mutex> lock(twist_mutex_);
    last_twist_message_ = msg;
    twist_received_ = true;
}