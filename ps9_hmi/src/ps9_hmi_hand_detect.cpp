//implementation of hmi class from library
//part of ps9_hmi package
//written by Matthew Dobrowsky

#include <ps9_hmi/ps9_hmi_hand_detect.h>

//library class constructor
HumanMachineInterface::HumanMachineInterface(ros::NodeHandle* nh) : cwru_pcl_utils(nh) {
	
	hand_present_ = false;

	//radius for detection area
	detect_radius_ = 0.075;
	
	//detected height of the hand
	hand_height_ = 10.0;

	//required ratio of present points to consider the hand "blocking"
	threshold_detection_ratio_ = 0.3;

	//ratio of blocked hand points
	blocked_ratio_ = 0.0;
}

//private function: polls kinect for current data 
void HumanMachineInterface::get_kinect_snapshot_() {
	cwru_pcl_utils.reset_got_kinect_cloud();
	ROS_INFO("HMI is getting a new kinect point cloud.");
	while(!cwru_pcl_utils.got_kinect_cloud()) {
		ROS_INFO("HMI is waiting on kinect point cloud.");
		ros::spinOnce();
		ros::Duration(1.0).sleep();
	}
	ROS_INFO("HMI is now processing a kinect point cloud.");

	//transform objects to move kinect pcl to torso frame
	tf::StampedTransform tf_kinect_to_torso;
	tf::TransformListener tf_listener;

	bool tferr = true;
    ROS_INFO("HMI is calculating a transform between the kinect and the torso.");
    while (tferr) {
        tferr = false;
        try {
            //look for a transform from kinect to the torso
            tf_listener.lookupTransform("torso", "kinect_pc_frame", ros::Time(0), tf_kinect_to_torso);
        } catch (tf::TransformException &exception) {
            ROS_ERROR("%s", exception.what());
            tferr = true;
            ros::Duration(0.5).sleep(); //sleep for half a second
            ros::spinOnce();
        }
    }
	ROS_INFO("HMI is executing a transform on the point cloud data.");

	//get eigen values for the transform
	Eigen::Affine3f eigen_kinect_to_torso;
	eigen_kinect_to_torso = cwru_pcl_utils.transformTFToEigen(tf_kinect_to_torso);

	cwru_pcl_utils.transform_kinect_cloud(eigen_kinect_to_torso);
	ROS_INFO("HMI has an updated and transformed kinect point cloud.");
}

//converts a point cloud XYZRGB to only XYZ
void HumanMachineInterface::convert_rgb_to_xyz_(PointCloud<pcl::PointXYZRGB> inputCloud, PointCloud<pcl::PointXYZ> outputCloud) {

    int npts = inputCloud.points.size(); //how many points to copy?

    outputCloud.header = inputCloud.header;
    outputCloud.is_dense = inputCloud.is_dense;
    outputCloud.width = inputCloud.width;
    outputCloud.height = inputCloud.height;

    outputCloud.points.resize(npts);
    for (int i = 0; i < npts; ++i) {
        outputCloud.points[i].getVector3fMap() = inputCloud.points[i].getVector3fMap();
    }
}

//public function: human interaction checking
bool HumanMachineInterface::get_human_hand() {
	get_kinect_snapshot_();

	//number of pcl points that were counted as "hand" in detection box
	double hand_point_count;
	//total number of pcl points in detection box
	double detected_area_point_count;

	//for points that are hand height
	vector<int> valid_height_index;
	PointCloud<pcl::PointXYZRGB> rgb_height_cloud;
	PointCloud<pcl::PointXYZ>::Ptr height_cloud;

	//list of indices for all points in detection radius
	vector<int> detected_index;
	//list of indices for points that are considered "hand"
	vector<int> hand_index;

	//gather all points of approximately hand height
	//stored in valid_height_index
	valid_height_index.clear();
	cwru_pcl_utils.find_coplanar_pts_z_height(HeightRough, HeightErr, valid_height_index);

	//find a rough centroid of the hand
	cwru_pcl_utils.copy_indexed_pts_to_output_cloud(valid_height_index, rgb_height_cloud);
	convert_rgb_to_xyz_(rgb_height_cloud, *height_cloud);
	hand_centroid_ = cwru_pcl_utils.compute_centroid(height_cloud);
	
	//use centroid to select a more precise set of points as hand
	//stored in hand_index
	cwru_pcl_utils.filter_cloud_z(HeightRough, HandErr, detect_radius_, hand_centroid_, hand_index);

	//select all points in the detection radius
	//stored in detected_index
	cwru_pcl_utils.filter_cloud_z(GeneralHeight, GeneralSpread, detect_radius_, hand_centroid_, detected_index);

	//get count values from index sizes
	hand_point_count = (double) hand_index.size();
	detected_area_point_count = (double) detected_index.size();
	
	//checks the ratio 
	blocked_ratio_ = hand_point_count / detected_area_point_count;
	if(blocked_ratio_ >= threshold_detection_ratio_) {
		hand_present_ = true;
	}
	else {
		hand_present_ = false;
	}

	return hand_present_;
}
