# ps9_gripper_control

contains library with 2 functions Gripper::open_hand() and Gripper::close_hand().

## Example usage

To run test code:

	1. baxter_master
	2. rosrun baxter_gripper dynamixel_motor_node
	3. open new window
	4. baxter_master
	5. rosrun ps9_gripper_control gripper_test

This code will open the gripper on launch then alternate between opening and closing the gripper every second
    
