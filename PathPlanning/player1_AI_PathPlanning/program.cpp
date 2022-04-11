
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>

#define KEY(c) ( GetAsyncKeyState((int)(c)) & (SHORT)0x8000 )

#include "image_transfer.h"

// include this header file for computer vision functions
#include "vision.h"

#include "robot.h"

#include "vision_simulation.h"

#include "timer.h"

extern robot_system S1;

// define a class to represent objects
// object_id_(1: self robot head, 2 : self robot rear
//            3: enemy head, 4: enemy rear
//            5: obstacles, 6: wheels)
class object {
public:
	object();
	object(double position_x, double position_y);
	// identify the object:
	// *input: rgb image, self robot colour, center of object
	// *return: object_id_
	int identify_object(image input_RGB_image,
		image label_map,
		int self_colour);
	bool calculate_robot_theta(object rear);
	double get_position_x();
	double get_position_y();
	double get_theta();
	int get_id();
	int get_label_value();
private:
	int object_id_, label_value_;
	double position_x_, position_y_, theta_;
};


object::object() {
	position_x_ = 0;
	position_y_ = 0;
	theta_ = 0;
}

object::object(double position_x, double position_y) :
	position_x_(position_x), position_y_(position_y) {};

int object::identify_object(image input_RGB_image,
							image label_map,
							int self_colour) {
	i2byte* p_label = (i2byte*)label_map.pdata;
	ibyte* p = input_RGB_image.pdata;
	int i = (int)position_x_;
	int j = (int)position_y_;
	//std::cout << "label: " << p_label[j * label_map.width + i] << std::endl;
	// calculate the position of pointer of object center
	int wheel_length = 6;
	ibyte* p_center = p + (i + input_RGB_image.width * j) * 3;
	ibyte* p_left = p + (i - wheel_length + input_RGB_image.width * j) * 3;
	ibyte* p_right = p + (i + wheel_length + input_RGB_image.width * j) * 3;
	ibyte* p_up = p + (i + input_RGB_image.width * (j + wheel_length)) * 3;
	ibyte* p_down = p + (i + input_RGB_image.width * (j - wheel_length)) * 3;
	// get the colour of object center
	ibyte B = *p_center;
	ibyte G = *(p_center + 1);
	ibyte R = *(p_center + 2);
	// get the colour at four directions larger than robot's wheel's size
	ibyte B_left = *p_left;
	ibyte B_right = *p_right;
	ibyte B_down = *p_down;
	ibyte B_up = *p_up;
	// check if is still the same colour at four directions
	// larger than robot's size
	int max_error = 15;
	int left_error = abs(B_left - B);
	int right_error = abs(B_right - B);
	int up_error = abs(B_up - B);
	int down_error = abs(B_down - B);
	//std::cout << "errors: " << left_error <<" "
	//						<< right_error << " "
	//						<< up_error << " "
	//						<< down_error << std::endl;
	if (left_error > max_error || right_error > max_error ||
		up_error > max_error || down_error > max_error) {
		//std::cout << "found a wheel" << std::endl;
		object_id_ = 6;
		label_value_ = p_label[j * label_map.width + i];
		return 6;
	}
	// get the colour at four directions larger than robot's size
	int robot_radius = 20;
	p_left = p + (i - robot_radius + input_RGB_image.width * j) * 3;
	p_right = p + (i + robot_radius + input_RGB_image.width * j) * 3;
	p_up = p + (i + input_RGB_image.width * (j + robot_radius)) * 3;
	p_down = p + (i + input_RGB_image.width * (j - robot_radius)) * 3;
	B_left = *p_left;
	B_right = *p_right;
	B_down = *p_down;
	B_up = *p_up;
	left_error = abs(B_left - B);
	right_error = abs(B_right - B);
	up_error = abs(B_up - B);
	down_error = abs(B_down - B);
	//std::cout << "errors: " << left_error << " "
	//	<< right_error << " "
	//	<< up_error << " "
	//	<< down_error << std::endl;
	// check if is still the same color at four directions
	// larger than robot's size
	if (self_colour == 1) {
		if (left_error > max_error || right_error > max_error ||
			up_error > max_error || down_error > max_error) {
			// check the colour of center to determine is self or enemy
			if (R > 200) {
				if (G > 150) { 
					object_id_ = 3; 
					label_value_ = p_label[j * label_map.width + i + robot_radius];
					return 3; 
				}
				else { 
					object_id_ = 2;
					label_value_ = p_label[j * label_map.width + i + robot_radius]; 
					return 2; }
			}
			else if (B > 200) { object_id_ = 4; 
			label_value_ = p_label[j * label_map.width + i + robot_radius]; 
			return 4; }
			else { object_id_ = 1; 
			label_value_ = p_label[j * label_map.width + i + robot_radius]; 
			return 1; }
		}
	}
	else {
		if (left_error > max_error || right_error > max_error ||
			up_error > max_error || down_error > max_error) {
			// check the colour of center to determine is self or enemy
			if (R > 200) {
				if (G > 150) { object_id_ = 1; label_value_ = p_label[j * label_map.width + i + robot_radius]; return 1; }
				else { object_id_ = 4; label_value_ = p_label[j * label_map.width + i + robot_radius]; return 4; }
			}
			else if (B > 200) { object_id_ = 2; label_value_ = p_label[j * label_map.width + i + robot_radius]; return 2; }
			else { object_id_ = 3; label_value_ = p_label[j * label_map.width + i + robot_radius]; return 3; }
		}
	}
	//std::cout << "found an obstacle" << std::endl;
	object_id_ = 5;
	label_value_ = p_label[j * label_map.width + i];
	return 5;
}

bool object::calculate_robot_theta(object rear) {
	double rear_x = rear.get_position_x();
	double rear_y = rear.get_position_y();
	double theta;
	/*std::cout << " rear_x: " << rear_x << std::endl;
	std::cout << " rear_y: " << rear_y << std::endl;
	std::cout << " x: " << position_x_ << std::endl;
	std::cout << " y: " << position_y_ << std::endl;
	std::cout << " position_y_ - rear_y: " << (position_y_ - rear_y) << std::endl;
	if ((position_y_ - rear_y) < 0) {
		theta = -atan2((rear_y - position_y_), (position_x_ - rear_x));
	}
	else{
		theta = atan2((position_y_ - rear_y), (position_x_ - rear_x));
	}*/
	theta = atan2((position_y_ - rear_y), (position_x_ - rear_x));
	theta_ = theta;
	return true;
}

double object::get_position_x() {
	return position_x_;
}

double object::get_position_y() {
	return position_y_;
}

double object::get_theta() {
	return theta_;
}

int object::get_id() {
	return object_id_;
}

int object::get_label_value() {
	return label_value_;
}

// get positions of two robots and map which includes all obstacles,
// return the number of found objects
int get_positions_from_image(image rgb, int self_colour,
	std::vector<object> & objects, image & label_map) {
	image temp_image, map;
	temp_image.type = GREY_IMAGE;
	temp_image.width = rgb.width;
	temp_image.height = rgb.height;
	map.type = GREY_IMAGE;
	map.width = rgb.width;
	map.height = rgb.height;
	allocate_image(map);
	allocate_image(temp_image);
	copy(rgb, temp_image);
	map.type = GREY_IMAGE;
	scale(temp_image, map);
	lowpass_filter(map, temp_image);
	threshold(temp_image, map, 70);
	invert(map, temp_image);
	erode(temp_image, map); // denoise
	dialate(map, temp_image);
	// label objects
	int nlabel;
	label_image(temp_image, label_map, nlabel);
	std::cout << "found " << nlabel << " objects" << std::endl;
	double ic[9], jc[9];
	for (int i = 1; i <= nlabel; i++) {
		centroid(temp_image, label_map, i, ic[i], jc[i]);
		object obj(ic[i], jc[i]);
		//std::cout << "object " << i << " position: " << ic[i] << " " << jc[i] << std::endl;
		objects.push_back(obj);
		objects[i-1].identify_object(rgb, label_map, self_colour);
		std::cout << "id: x,y,label: " << objects[i - 1].get_id() << ": " <<
			ic[i] << " " << jc[i] << " " << objects[i - 1].get_label_value()<< std::endl;
		//draw_point_rgb(rgb, ic[i], jc[i], 255, 0, 0);
		//view_rgb_image(rgb);
	}

	free_image(temp_image);
	free_image(map);
	return true;
}

// check whether a pixel is part of obstacls
// return true if it is free
bool check_space(std::vector<object> objects, int i, int j) {
	double obstacle_radius = 50 + 40;
	for (int k = 0; k < objects.size(); k++) {
		if (objects[k].get_id() == 5) {
			double distance = sqrt((i - objects[k].get_position_x()) * (i - objects[k].get_position_x())+
				(j - objects[k].get_position_y()) * (j - objects[k].get_position_y()));
			if (distance < obstacle_radius) {
				return false;
			}
		}
	}
	return true;
}

bool get_robots(std::vector<object> objects, object& self, object& self_rear,
				object& enemy, object& enemy_rear) {
	for (int i = 0; i < objects.size(); i++) {
		if (objects[i].get_id() == 1) {
			self = objects[i];
		}
		if (objects[i].get_id() == 3) {
			enemy = objects[i];
		}
		if (objects[i].get_id() == 2) {
			self_rear = objects[i];
		}
		if (objects[i].get_id() == 4) {
			enemy_rear = objects[i];
		}
	}
	return true;
}

//bool get_robot_position(std::vector<object> objects, 
//	double &self_position_x, double &self_position_y, double &self_position_theta,
//	double &enemy_position_x, double &enemy_position_y, double &enemy_position_theta) {
//	object self, self_rear, enemy, enemy_rear;
//	for (int i = 0; i < objects.size(); i++) {
//		switch (objects[i].get_id()) {
//		case 1: {
//			self = objects[i];
//			self_position_x = objects[i].get_position_x();
//			self_position_y = objects[i].get_position_y();
//		}
//		case 2: {
//			self_rear = objects[i];
//		}
//		case 3: {
//			enemy = objects[i];
//			enemy_position_x = objects[i].get_position_x();
//			enemy_position_y = objects[i].get_position_y();
//		}
//		case 4: {
//			enemy_rear = objects[i];
//		}
//		}
//	}
//	self.calculate_robot_theta(self_rear);
//	self_position_theta = self.get_theta();
//	enemy.calculate_robot_theta(enemy_rear);
//	enemy_position_theta = enemy.get_theta();
//	return true;
//}

////part 1------------The Funciton Defination for Path planning------------Starts
// Author  : Xiaobo Wu
// Data    : 2022.04.11/edition-1
// Function: path planning part include four subfunctions: 
// 
// 1.SamplingArray: Create Sample point in Body coordinate system;
// 2.RotationArray: Transform Sample point from Body coordinate system to Globle coordinate system
// 3.Get_Angle_Rotation: Get part angle for transform of rotation Array 

//------------------1.SamplingArray: Create Sample point from Body coordinate system-------Start
void SamplingArray(float Sampling_points[(270 / 5 + 1) * 3], float Mini_Radian, float S_R)
/////////////////////////////////////////////////////////////////////////////////////
// Input :  Mini_Radian: Minimum radian interval for search
//          S_R        : Sample Search Step size  
// Output:  Sampling point and turn angle in body coordinate system 
// 
// Notes for Output: from first element to third element is the first sampling group ;
// and from fourth element to sixthe element is the second sampling group in array Sampling_points;
/////////////////////////////////////////////////////////////////////////////////////
{
	Sampling_points[0] = S_R;
	Sampling_points[1] = 0;
	Sampling_points[2] = 0;
	//cout << Sampling_points[0] << Sampling_points[1] << Sampling_points[2];
	//cout << "\n";
	int i_R = 1;
	for (int ii = 1; ii < (270 / 5); ii++)
	{
		Sampling_points[3 * ii] = S_R * cos(Mini_Radian * i_R);
		Sampling_points[3 * ii + 1] = S_R * sin(Mini_Radian * i_R);
		Sampling_points[3 * ii + 2] = Mini_Radian * i_R;

		Sampling_points[3 * ii + 3] = +Sampling_points[3 * ii];
		Sampling_points[3 * ii + 4] = -Sampling_points[3 * ii + 1];
		Sampling_points[3 * ii + 5] = -Sampling_points[3 * ii + 2];

		//		cout << Sampling_points[3 * ii] <<" ;" << Sampling_points[3 * ii + 1] << " ;" << Sampling_points[3 * ii + 2];
		//		cout << "\n";

		ii++;
		i_R = i_R + 1;
	}
}
//------------------2.RotationArray: Transform Sample point------------------
int TF_X(float Reason[2], int Translation_x, float Radian)
{
	// Input:
	//	Reason[2]:      Position to be converted  in the body coordinate system
	//	Translation[2]: Translation or the current position of the object in map coordinate system
	//	Radian:         Rotation angle from body coordinate system to map coordinate system(clockwise)
	// Output:
	//	Result_x:       Is the x value be converted in the map coordinate system
	int Result_x;
	Result_x = cos(Radian) * Reason[0] - sin(Radian) * Reason[1] + Translation_x;
	return Result_x;
}
int TF_Y(float Reason[2], int Translation_y, float Radian)
{
	// Input:
	//	Reason[2]:      Position to be converted  in the body coordinate system
	//	Translation[2]: Translation or the current position of the object in map coordinate system
	//	Radian:         Rotation angle from body coordinate system to map coordinate system(clockwise)
	// Output:
	//	Result_y:       Is the y value be converted in the map coordinate system
	int Result_y;
	Result_y = sin(Radian) * Reason[0] + cos(Radian) * Reason[1] + Translation_y;
	return Result_y;
}

//----------------- 3.Get the angle from Start_A(Current)and Goal_B in the map coordinate system----------
float Get_Angle_Rotation(int Start[2], int Goal[2])
/////////////////////////////////////////////////////////////////////////////////////
// Input: Start(Current)Point  A and Goal Point B
// Output: the angle of from A to B (0 <= angle < 2 * 3.1415926)
/////////////////////////////////////////////////////////////////////////////////////
{
	float Radian_Temple;
	Radian_Temple = atan(abs((Goal[1] - Start[1]) / (Goal[0] - Start[0])));

	if (((Goal[1] - Start[1]) > 0) && ((Goal[0] - Start[0]) > 0))
	{
		return Radian_Temple;
	}
	if (((Goal[1] - Start[1]) > 0) && ((Goal[0] - Start[0]) < 0))
	{
		return Radian_Temple + 3.1415926 / 2;
	}
	if (((Goal[1] - Start[1]) < 0) && ((Goal[0] - Start[0]) < 0))
	{
		return Radian_Temple + 3.1415926;
	}
	if (((Goal[1] - Start[1]) < 0) && ((Goal[0] - Start[0]) > 0))
	{
		return 2 * 3.1415926 - Radian_Temple;
	};
}
////1.part-------The Funciton Defination for Path planning --------End

int main()
{
	double x0, y0, theta0, max_speed, opponent_max_speed;
	int pw_l, pw_r, pw_laser, laser;
	double light, light_gradient, light_dir, image_noise;
	double width1, height1;
	int N_obs, n_robot;
	double x_obs[50], y_obs[50], size_obs[50];
	double D, Lx, Ly, Ax, Ay, alpha_max;
	double tc, tc0; // clock time
	int mode, level;
	int pw_l_o, pw_r_o, pw_laser_o, laser_o;

	// TODO: it might be better to put this model initialization
	// section in a separate function

	// note that the vision simulation library currently
	// assumes an image size of 640x480

	width1 = 640;
	height1 = 480;
	// number of obstacles

	N_obs = 2;
	x_obs[1] = 300; // pixels
	y_obs[1] = 200; // pixels
	size_obs[1] = 1.0; // scale factor 1.0 = 100% (not implemented yet)	

	x_obs[2] = 300; // pixels
	y_obs[2] = 300; // pixels
	size_obs[2] = 1.0; // scale factor 1.0 = 100% (not implemented yet)	

	// set robot model parameters ////////

	D = 121.0; // distance between front wheels (pixels)

	// position of laser in local robot coordinates (pixels)
	// note for Lx, Ly we assume in local coord the robot
	// is pointing in the x direction		
	Lx = 31.0;
	Ly = 0.0;

	// position of robot axis of rotation halfway between wheels (pixels)
	// relative to the robot image center in local coordinates
	Ax = 37.0;
	Ay = 0.0;

	alpha_max = 3.14159 / 2; // max range of laser / gripper (rad)

	// number of robot (1 - no opponent, 2 - with opponent, 3 - not implemented yet)
	n_robot = 2;

	std::cout << "\npress space key to begin program.";
	pause();

	// you need to activate the regular vision library before 
	// activating the vision simulation library
	activate_vision();

	// note it's assumed that the robot points upware in its bmp file

	// however, Lx, Ly, Ax, Ay assume robot image has already been
	// rotated 90 deg so that the robot is pointing in the x-direction
	// -- ie when specifying these parameters assume the robot
	// is pointing in the x-direction.

	// note that the robot opponent is not currently implemented in 
	// the library, but it will be implemented soon.

	activate_simulation(width1, height1, x_obs, y_obs, size_obs, N_obs,
		"robot_A.bmp", "robot_B.bmp", "background.bmp", "obstacle.bmp", D, Lx, Ly,
		Ax, Ay, alpha_max, n_robot);

	// open an output file if needed for testing or plotting
//	ofstream fout("sim1.txt");
//	fout << scientific;

	// set simulation mode (level is currently not implemented)
	// mode = 0 - single player mode (manual opponent)
	// mode = 1 - two player mode, player #1
	// mode = 2 - two player mode, player #2	
	mode = 1;
	level = 1;
	set_simulation_mode(mode, level);

	// set robot initial position (pixels) and angle (rad)
	x0 = 150;
	y0 = 350;
	theta0 = 1;
	set_robot_position(x0, y0, theta0);

	// set initial inputs / on-line adjustable parameters /////////

	// inputs
	// pw_l -- pulse width of left servo (us) (from 1000 to 2000)
	// pw_r -- pulse width of right servo (us) (from 1000 to 2000)
	pw_l = 1500; // pulse width for left wheel servo (us)
	pw_r = 1500; // pulse width for right wheel servo (us)
	pw_laser = 1500; // pulse width for laser servo (us)
	laser = 0; // laser input (0 - off, 1 - fire)

	// paramaters
	max_speed = 100; // max wheel speed of robot (pixels/s)
	opponent_max_speed = 100; // max wheel speed of opponent (pixels/s)

	// lighting parameters (not currently implemented in the library)
	light = 1.0;
	light_gradient = 1.0;
	light_dir = 1.0;
	image_noise = 1.0;

	// set initial inputs
	set_inputs(pw_l, pw_r, pw_laser, laser,
		light, light_gradient, light_dir, image_noise,
		max_speed, opponent_max_speed);

	image rgb;
	int height, width;

	// note that the vision simulation library currently
	// assumes an image size of 640x480
	width = 640;
	height = 480;

	rgb.type = RGB_IMAGE;
	rgb.width = width;
	rgb.height = height;

	// allocate memory for the images
	allocate_image(rgb);

	wait_for_player();

	// measure initial clock time
	tc0 = high_resolution_time();
	std::cout << "Start" << std::endl;

	//part 2.//////////-------------Data Initialization for Path planing----------------Start
/// Author  : Xiaobo Wu
//  Data    : 2022.04.10 

	// Create  circle obstacle in [100,100] with radium 30;
	int Obstacle_Centre1[2] = { x_obs[1],y_obs[1] };
	int Obstacle_Centre2[2] = { x_obs[2],x_obs[2] };
	int Step_Long = 20;// Can not bigger the radium of obstacle 2~5
	// Intermediate variable
	float Sample_Point_Body[2];
	float Sample_Theta_Body;
	int Sample_Point_Global[2];
	float Sample_Theta_Global;

	float Radian_Get;
	int Robot_Global_Current_Position[2];

	float* S = new float[(270 / 5 + 1) * 3];
	SamplingArray(S, 3.1415926 / 19, Step_Long);   // Sampling points be stored in S array in the body coodination
	int A_Global_Start[2] = { x0, y0 };
	int B_Global_End[2];
	//part 2.//////////-------------Data Initialization for Path planing----------------End


	while (1) {

		// simulates the robots and acquires the image from simulation
		acquire_image_sim(rgb);

		tc = high_resolution_time() - tc0;

		// change the inputs to move the robot around
		// or change some additional parameters (lighting, etc.)
		// only the following inputs work so far
		// pw_l -- pulse width of left servo (us) (from 1000 to 2000)
		// pw_r -- pulse width of right servo (us) (from 1000 to 2000)
		// pw_laser -- pulse width of laser servo (us) (from 1000 to 2000)
		// -- 1000 -> -90 deg
		// -- 1500 -> 0 deg
		// -- 2000 -> 90 deg
		// laser -- (0 - laser off, 1 - fire laser for 3 s)
		// max_speed -- pixels/s for right and left wheels
		set_inputs(pw_l, pw_r, pw_laser, laser,
			light, light_gradient, light_dir, image_noise,
			max_speed, opponent_max_speed);

		//   Image processing --------------------------------------------------

		// The colour of our own robot which should be known, 1 represents A
		// 2 represents B
		int self_colour = 1;
		std::vector<object> objects;
		image label_map;
		label_map.type = LABEL_IMAGE;
		label_map.width = rgb.width;
		label_map.height = rgb.height;
		allocate_image(label_map);
		int nlabel = get_positions_from_image(rgb, self_colour, objects, 
											label_map);
		object self,self_rear,enemy,enemy_rear;
		get_robots(objects,self,self_rear,enemy,enemy_rear);
		self.calculate_robot_theta(self_rear);
		enemy.calculate_robot_theta(enemy_rear);
		double self_position_x, self_position_y, self_position_theta,
			enemy_position_x, enemy_position_y, enemy_position_theta;
		self_position_x = self.get_position_x();
		self_position_y = self.get_position_y();
		self_position_theta = self.get_theta();
		enemy_position_x = enemy.get_position_x();
		enemy_position_y = enemy.get_position_y();
		enemy_position_theta = enemy.get_theta();
		// print the output
		std::cout << "self_position: " << self_position_x << ", "
			<< self_position_y << ", "
			<< self_position_theta 
			<< ", " << std::endl;
		std::cout << "enemy_position: " << enemy_position_x << ", "
			<< enemy_position_y << ", "
			<< enemy_position_theta 
			<< ", " << std::endl;
		//copy(map, rgb);
		draw_point_rgb(rgb, self_position_x, self_position_y, 0, 0, 255);
		draw_point_rgb(rgb, enemy_position_x, enemy_position_y, 0, 255, 0);

		std::cout << "is 100, 100 free? "
			<< check_space(objects, 320, 400) << std::endl;
		std::cout << "is 300, 200 free? "
			<< check_space(objects, 300, 200) << std::endl;

		////part 3.------------------Track object for Path planning-----------------Start
		/// Author  : Xiaobo Wu
		/// Data    : 2022.04.10 
		int B_Global_End[2] = { enemy_position_x, enemy_position_y };// Set goal/end point
		for (int j = 0; j < 270 / 5 + 1; j++)
		{
			//initialize the point to be transform
			Sample_Point_Body[0] = S[j];
			Sample_Point_Body[1] = S[j + 1];
			Sample_Theta_Body = S[j + 2];
			// Initialize the Rotation angle value from A TO B point in the global coordinate system
			Radian_Get = Get_Angle_Rotation(A_Global_Start, B_Global_End) + Sample_Theta_Body;
			// Transform body coordinate system to global coordinte system
			Sample_Point_Global[0] = TF_X(Sample_Point_Body, A_Global_Start[0], Radian_Get);
			Sample_Point_Global[1] = TF_Y(Sample_Point_Body, A_Global_Start[1], Radian_Get);
			if (check_space(objects, Sample_Point_Global[0], Sample_Point_Global[1]) == true)
			{
				/* be used to rebug , do not move it
						cout << "get veiw point/ Global positon:";
				cout << Sample_Point_Global[0] << ";" << Sample_Point_Global[1];
				cout << "\n";
				cout << " And Body Position And Theta :";
				cout << Sample_Point_Body[0] << ";" << Sample_Point_Body[1] << ";" << 180 * Sample_Theta_Body / 3.1415926;
				cout << "\n";
				cout << "Position A , Angle of A TO B Theta, Angle of Rotation:";
				cout << A_Global_Start[0] << ";"<< A_Global_Start[1];
				cout << ";" << 180 * Get_Angle_Rotation(A_Global_Start, B_Global_End) / 3.1415926 << ";" << 180 * Radian_Get / 3.1415926;
				cout << "\n";
				*/

				A_Global_Start[0] = Sample_Point_Global[0];                           // update x label of start point 
				A_Global_Start[1] = Sample_Point_Global[1];                           // update y label of start point
				//Check_Local_Min[kkk] = { A_Global_Start[0], A_Global_Start[1] };
				//kkk++;
				draw_point_rgb(rgb, A_Global_Start[0], A_Global_Start[1], 225, 0, 0); // draw the point to track/draw the goal point
				break;
			}
			j = j + 2;
		}
		////part 3.------------------Track object for Path planning-----------------END

		//free_image(map);
		//   Image processing done ---------------------------------------------

		// NOTE: only one program can call view_image()
		view_rgb_image(rgb);

		// don't need to simulate too fast
		Sleep(500); // 100 fps max
	}

	// free the image memory before the program completes
	free_image(rgb);

	deactivate_vision();

	deactivate_simulation();

	std::cout << "\ndone.\n";

	return 0;
}