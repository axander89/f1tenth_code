/**********************************
**Using ZED with OpenCV
**********************************/
#include <iostream>
#include <vector>
#include <sstream>
// ROS
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <race/drive_param.h>
#include <std_msgs/String.h> // FOR DEBUG ONLY
// OpenCV
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

// Global vars
#define MIN_RERROR 120
#define MAX_RERROR 160 // if closer than this radius, reverse
int radius_error = 0;
int center_error = 0;
int hueMinValue = 8/2;
int hueMaxValue = 14/2;
int satMinValue = 100;
int satMaxValue = 255;
int volMinValue = 50;
int volMaxValue = 255;
float Kp = 0.8;
float Kd = 0;
float Ki = 0;
float freq = 10;
float last_error = 0;
float center_integral = 0;
size_t count = 0;

// Set up reference object
cv::Point2i ref_center(573,374);
int ref_radius = 120;
float minTargetRadius = 80;

race::drive_param pwm_msg;

// Input from keyboard
char keyboard = ' ';

class ImageConverter
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  ros::Publisher pwm_pub_;
  ros::Timer timer;

public:
  ImageConverter(): it_(nh_)
  {
    // Subscrive to input video feed and publish output drive pwm values
    image_sub_ = it_.subscribe("/zed/rgb/image_raw_color",1,&ImageConverter::imageCb,this);
    pwm_pub_ = nh_.advertise<race::drive_param>("drive_parameters", 10);
    timer = nh_.createTimer(ros::Duration(0.1), &ImageConverter::controlCb,this); // timer cb every 0.1 sec
    
    // Create OpenCV windows
    cv::namedWindow("Image", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Threshold Image", cv::WINDOW_AUTOSIZE);
  }

  ~ImageConverter()
  {
    cv::destroyWindow("Image");
    cv::destroyWindow("Threshold Image");
  }
  
  void controlCb(const ros::TimerEvent&)
  {
	pwm_pub_.publish(pwm_msg);
  }

  void imageCb(const sensor_msgs::ImageConstPtr& msg)
  {
    // Get image from ZED camera (from rostopic)
    cv_bridge::CvImagePtr cv_ptr;
    try
    {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }

    // Convert BRG to HSV
    cv::Mat HSV_image;
    cv::cvtColor(cv_ptr->image,HSV_image,cv::COLOR_BGR2HSV);

    // Create threshold frame of the image
    cv::Mat threshold_frame;
    cv::inRange(HSV_image, cv::Scalar(hueMinValue, satMinValue, volMinValue), cv::Scalar(hueMaxValue, satMaxValue, volMaxValue), threshold_frame);

    // Filter out extra noise
    cv::Mat str_el = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    morphologyEx(threshold_frame, threshold_frame, cv::MORPH_OPEN, str_el);
    morphologyEx(threshold_frame, threshold_frame, cv::MORPH_CLOSE, str_el);

    // Get circle around object
    cv::vector<cv::vector<cv::Point> > contours;
    cv::vector<cv::Vec4i> hierarchy;
    cv::vector<cv::Point2i> center;
    cv::vector<int> radius;
    cv::findContours(threshold_frame.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

    count = contours.size();
     
    for(int i=0; i<count; i++){
        cv::Point2f c;
        float r;
        cv::minEnclosingCircle(contours[i], c, r);
     
        if (r >= minTargetRadius){
            center.push_back(c);
            radius.push_back(r);
        }
    }

    // Number of contours detected
    count = center.size();
    if (count == 1){
        // Draw circle on threshold image
        for(int i = 0; i < count; i++){
            cv::circle(threshold_frame, center[i], radius[i], cv::Scalar(255,0,0), 3);
        }

        // Control section
	last_error = center_error;
        radius_error = radius[0]-ref_radius;
        center_error = center[0].x-ref_center.x;
	center_integral = center_integral + center_error;
	
	if (radius_error > MAX_RERROR){
		pwm_msg.velocity = 9480;
	}
	else if (radius_error < MIN_RERROR){
		pwm_msg.velocity = 10080;
	}
	else{
		pwm_msg.velocity = 9780;
	}
	float pid;
	pid = (Kp*center_error+Kd*(center_error-last_error)*freq+Ki*center_integral)*(10)/(600);
        pwm_msg.angle = (int)pid;
    }
    // If no/multiple points found, default action is to halt
    else{
        pwm_msg.velocity = 9780; // pwm_drive are true pwm values
        pwm_msg.angle = 0; // pwm_angle [-10,10]
    }

    // Update GUI Window
    cv::imshow("Image", cv_ptr->image);
    cv::imshow("Threshold Image", threshold_frame);
    cv::waitKey(30);
    
    // Output new driving parameter values
    //pwm_pub_.publish(pwm_msg);
  }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "objectTracker");
  ImageConverter ic;
  ros::spin();
  return 0;
}
