
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <stdio.h>
#include <cmath>  

#include "AsioTelnetClient.h"
#include <time.h>
#include <dos.h>
#include <windows.h>
#include <stdlib.h>

#ifdef POSIX
#include <termios.h>
#endif

using namespace std;
using namespace cv;

Point detectAndDisplay(Mat frame);
String trackIt(Point meta, double frameWidth, double frameHeight);

Mat frame; //frame to detect faces on
CascadeClassifier cascade; //define cascade
string cascade_name = "haarcascade_frontalface.xml"; //pick cascade here

int main(int argc, char * argv[])
{
	#ifdef POSIX
		termios stored_settings;
		tcgetattr(0, &stored_settings);
		termios new_settings = stored_settings;
		new_settings.c_lflag &= (~ICANON);
		new_settings.c_lflag &= (~ISIG); // don't automatically handle control-C
		tcsetattr(0, TCSANOW, &new_settings);
	#endif

	string dest_ip;
	string dest_port;

	if (argc != 3)
	{
		#ifdef WIN32
				dest_ip = "192.168.0.1";
				dest_port = "23";
		#else
				std::cerr << "Usage: telnet <host> <port>\n";
				return 1;
		#endif
	}
	else
	{
		dest_ip = argv[1];
		dest_port = argv[2];
	}

	cout << "SimpleTelnetClient is tring to connect " << dest_ip << ":" << dest_port << std::endl;

	boost::asio::io_service io_service;

	// resolve the host name and port number to an iterator that can be used to connect to the server
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(dest_ip, dest_port);
	tcp::resolver::iterator iterator = resolver.resolve(query);
	// define an instance of the main class of this program

	AsioTelnetClient telnet_client(io_service, iterator);

	telnet_client.setReceivedSocketCallback([](const std::string& message) {
		std::cout << message;
	});

	telnet_client.setClosedSocketCallback([]() {
		std::cout << " # disconnected" << std::endl;
	});


	//open video and cascade + error check
	VideoCapture capture(0);

	if (!capture.isOpened()) 
		{ printf("Error loading video capture"); return 1; }
	if (!cascade.load(cascade_name)) 
		{ printf("Error loading cascade"); return 1; }

	double frameWidth = capture.get(CAP_PROP_FRAME_WIDTH);
	double frameHeight = capture.get(CAP_PROP_FRAME_HEIGHT);
	double resolution = frameWidth * frameHeight;
	
	String str;
	Point center;

	telnet_client.write('a');
	telnet_client.write('d');
	telnet_client.write('m');
	telnet_client.write('i');
	telnet_client.write('n');

	telnet_client.write('\r');

	Sleep(500);

	telnet_client.write('p');
	telnet_client.write('a');
	telnet_client.write('s');
	telnet_client.write('s');
	telnet_client.write('w');
	telnet_client.write('o');
	telnet_client.write('r');
	telnet_client.write('d');

	telnet_client.write('\r');

	int j = 0;

	while(true)
	{
		capture >> frame; //grab frame from video capture
		j++;

		if (!frame.empty())
		{
			center = detectAndDisplay(frame);

			if (j % 2 == 0)
			{
				str = trackIt(center, frameWidth, frameHeight);

				for (std::string::size_type i = 0; i < str.size(); ++i)
				{
					cout << str[i];
					telnet_client.write(str[i]);
				}
				telnet_client.write('\r');
			}
		}
		else
		{
			printf("Error capturing frame");
			break;
		}

		if (27 == char(waitKey(10))) break; //wait for ESC key to exit
	}

	#ifdef POSIX // restore default buffering of standard input
	tcsetattr(0, TCSANOW, &stored_settings);
	#endif

	return 0;
}

//detects faces and returns their location as a point
Point detectAndDisplay(Mat frame)
{
	vector<Rect> faces; //list of faces in frame
	Mat frame_gray; //grayscale frame
	Point botLeft; //point of rectangle
	Point topRight; //point of rectangle
	Point center; //center of rect
	int size; //size of face

	cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
	//params: 1-frame 2-vector 3-scale 4-minNeighbors 5-flags 6-minsize 7-maxsize
	cascade.detectMultiScale(frame_gray, faces, 1.2, 3, 0, Size(100, 100));

	//iterate through all detected faces and draw rectangles on frame
	for (int i = 0; i < faces.size(); i++)
	{
		botLeft = Point(faces[i].x, faces[i].y);
		topRight = Point((faces[i].x + faces[i].width), (faces[i].y + faces[i].height));
		center = Point((faces[i].x + 0.5 * faces[i].width), (faces[i].y + 0.5 * faces[i].height));
		size = (faces[i].width * faces[i].height);
		rectangle(frame, botLeft, topRight, Scalar(0, 255, 0), 2, 8, 0);
	}
	imshow("Automated Lecture Tracking", frame);
	return center;
}

String trackIt(Point meta, double frameWidth, double frameHeight)
{
	//double x = abs(48 * (meta.x / frameWidth - 0.5));
	//int speed = (int)x;

	if (meta.x == 0)
	{
		return "camera pan stop";
	}
	else if (meta.x < frameWidth * 0.4)
	{
		//cout << "camera pan left";
		return "camera pan left";
	}
	else if (meta.x > frameWidth * 0.6)
	{
		//cout << "camera pan right";
		return "camera pan right";
	}
	else
	{
		return "camera pan stop";
	}
}