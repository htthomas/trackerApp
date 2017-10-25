#include "AsioTelnetClient.h"
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <stdio.h>
#include <sstream>
#include <cstring>
#include <cmath>
#include <time.h>
#include <dos.h>
#include <windows.h>
#include <stdlib.h>

#ifdef POSIX
#include <termios.h>
#endif

using namespace std;
using namespace cv;

Rect detectAndDisplay(Mat frame);
String trackIt(Rect face, double frameWidth, double frameHeight, double size);

Mat frame; //frame to detect faces on
CascadeClassifier cascade;
string cascade_name = "headshoulders.xml"; //pick cascade here

int main(int argc, char * argv[])
{
	#ifdef POSIX
		termios stored_settings;
		tcgetattr(0, &stored_settings);
		termios new_settings = stored_settings;
		new_settings.c_lflag &= (~ICANON);
		new_settings.c_lflag &= (~ISIG);
		tcsetattr(0, TCSANOW, &new_settings);
	#endif

	string dest_ip;
	string dest_port;
	string acct;
	string pw;

	if (argc != 5)
	{
		#ifdef WIN32
			dest_ip = "137.229.182.171";
			dest_port = "23";
			acct = "admin";
			pw = "password";

		#else
			std::cerr << "Usage: autotrack <host> <port> <acct> <pw>\n";
			return 1;
		#endif
	}
	else
	{
		dest_ip = argv[1];
		dest_port = argv[2];
		acct = argv[3];
		pw = argv[4];
	}

	cout << "Trying to connect " << dest_ip << ":" << dest_port << std::endl;

	boost::asio::io_service io_service;

	tcp::resolver resolver(io_service);
	tcp::resolver::query query(dest_ip, dest_port);
	tcp::resolver::iterator iterator = resolver.resolve(query);

	AsioTelnetClient telnet_client(io_service, iterator);

	telnet_client.setReceivedSocketCallback([](const std::string& message)
	{
		std::cout << message;
	});

	telnet_client.setClosedSocketCallback([]()
	{
		std::cout << "Could not connect to host" << std::endl;
	});

	VideoCapture capture(0);

	if (!capture.isOpened())
	{
		printf("Error loading video capture"); return 1;
	}
	if (!cascade.load(cascade_name))
	{
		printf("Error loading cascade"); return 1;
	}

	double frameWidth = capture.get(CAP_PROP_FRAME_WIDTH);
	double frameHeight = capture.get(CAP_PROP_FRAME_HEIGHT);
	double size = frameWidth * frameHeight;

	//enter admin and password
	telnet_client.write(acct);
	telnet_client.write("\r");
	Sleep(500);
	telnet_client.write(pw);
	telnet_client.write("\r");

	int j = 0;
	string str;
	Rect face;
	Point center;
	string prv_cmd = "camera pan stop";
	POINT prv_center;
	prv_center.x = 0;
	prv_center.y = 0;

	while (true)
	{
		capture >> frame; //grab frame from video capture
		j++;

		if (!frame.empty())
		{
			face = detectAndDisplay(frame);
			center.x = face.x + face.width / 2;
			center.y = face.y + face.height / 2;

			if (center.x == 0 && center.y == 0) {
				center.x = prv_center.x;
				center.y = prv_center.y;
			}
			if (j % 5 == 0)
			{
				str = trackIt(face, frameWidth, frameHeight, size);
				if (str != prv_cmd)
				{
					telnet_client.write(str);
					telnet_client.write('\r');
				}
				prv_cmd = str;
			}

			prv_center.x = center.x;
			prv_center.y = center.y;
		}
		else
		{
			printf("Error capturing frame");
			break;
		}

		if (27 == char(waitKey(10))) break; //wait for ESC key to exit
	}

#ifdef POSIX
	tcsetattr(0, TCSANOW, &stored_settings);
#endif

	return 0;
}

//detect faces and return their location as a point
Rect detectAndDisplay(Mat frame)
{
	vector<Rect> faces; //list of faces in frame
	Mat frame_gray; //grayscale frame
	Point botLeft; //point of rectangle
	Point topRight; //point of rectangle
	Point center; //center of rect
	Rect faceRect;
	int size; //size of face

	cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
	//params: 1-frame 2-vector 3-scale 4-minNeighbors 5-flags 6-minsize 7-maxsize
	cascade.detectMultiScale(frame_gray, faces, 1.2, 3, CV_HAAR_FIND_BIGGEST_OBJECT, Size(100, 100));

	//iterate through all detected faces and draw rectangles on frame
	for (int i = 0; i < faces.size(); i++)
	{
		botLeft = Point(faces[i].x, faces[i].y);
		topRight = Point((faces[i].x + faces[i].width), (faces[i].y + faces[i].height));
		center = Point((faces[i].x + 0.5 * faces[i].width), (faces[i].y + 0.5 * faces[i].height));
		faceRect = Rect(botLeft, topRight);
		size = (faces[i].width * faces[i].height);
		rectangle(frame, botLeft, topRight, Scalar(0, 255, 0), 2, 8, 0);
	}
	imshow("Automated Lecture Tracking", frame);
	return faceRect;
}

//returns command to send to camera
String trackIt(Rect face, double frameWidth, double frameHeight, double size)
{
	Point meta = Point(face.x + face.width / 2, face.y + face.height / 2);
	int x_weight = (int)abs(200 * (meta.x / frameWidth - 0.5));
	int y_weight = (int)abs(200 * (meta.y / frameWidth - 0.5));
	double lowerLimit = 0.4;
	double upperLimit = 0.6;

	if ((meta.x > frameWidth*lowerLimit && meta.x < frameWidth*upperLimit) || meta.x == 0) {
		x_weight = 0;
	}
	if ((meta.y > frameHeight*lowerLimit && meta.y < frameHeight*upperLimit) || meta.y == 0) {
		y_weight = 0;
	}

	if (x_weight >= y_weight && x_weight > 0) {
		if (meta.x - frameWidth / 2 > 0) {
			return "camera pan right";
		}
		else {
			return "camera pan left";
		}
	}
	else if (x_weight < y_weight && y_weight > 0) {
		if (meta.y - frameHeight / 2 > 0) {
			return "camera tilt down";
		}
		else {
			return "camera tilt up";
		}
	}
	else if (x_weight == 0 && y_weight == 0) {
		if (abs(meta.x - frameWidth / 2) > abs(meta.y - frameHeight / 2)) {
			return "camera tilt stop";
		}
		else {
			return "camera pan stop";
		}
	}

	/*if (meta.x == 0)
	{
	return "camera pan stop";
	}
	if (meta.y == 0)
	{
	return "camera tilt stop";
	}
	if (face.width > frameWidth / 3 && face.width < frameWidth / 2) {
	return "camera zoom stop";
	}
	else if (meta.x < frameWidth * 0.3)
	{
	//cout << "camera pan left";
	return "camera pan left";
	}
	else if (meta.x > frameWidth * 0.7)
	{
	//cout << "camera pan right";
	return "camera pan right";
	}
	else if (meta.y < frameHeight *0.3)
	{
	return "camera tilt up";
	}
	else if (meta.y > frameHeight *0.7)
	{
	return "camera tilt down";
	}
	else if (face.width < frameWidth / 3) {
	return "camera zoom in 2";
	}
	else if (face.width > frameWidth / 2) {
	return "camera zoom out 2";
	}
	else {
	return "camera zoom stop";
	}*/
}