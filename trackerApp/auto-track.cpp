
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
String trackIt(Rect face, double frameWidth, double frameHeight, double frameSize);

Mat frame; //frame to detect faces on
CascadeClassifier cascade;
string cascade_name = "headshoulders.xml"; //specify cascade here

int main(int argc, char * argv[])
{
	//***begin telnet initialization***//
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
				dest_ip = "137.229.182.200";
				dest_port = "23";
				acct = "admin";
				pw = "password";

			#else
				cerr << "Usage: autotrack <host> <port> <acct> <pw>\n";
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

		cout << "Trying to connect " << dest_ip << ":" << dest_port << " " << acct << " " << pw << endl;

		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(dest_ip, dest_port);
		tcp::resolver::iterator iterator = resolver.resolve(query);
		AsioTelnetClient telnet_client(io_service, iterator);

		telnet_client.setReceivedSocketCallback([](const string& message)
		{
			cout << message;
		});

		telnet_client.setClosedSocketCallback([]()
		{
			cout << "Could not connect to host" << endl;
		});

	//***end telnet initialization***//

	Sleep(1000);
	telnet_client.write(acct);
	telnet_client.write("\r");
	Sleep(500);
	telnet_client.write(pw);
	telnet_client.write("\r");
	Sleep(500);

	VideoCapture capture(0);

	if (!capture.isOpened()){ printf("Error loading video capture"); return 1; }
	if (!cascade.load(cascade_name)){ printf("Error loading cascade"); return 1; }

	double frameWidth = capture.get(CAP_PROP_FRAME_WIDTH);
	double frameHeight = capture.get(CAP_PROP_FRAME_HEIGHT);
	double frameSize = frameWidth * frameHeight;

	Rect face;
	Rect prvface = Rect(Point(0, 0), Point(0, 0));
	string cmd;
	string prvcmd = "camera pan stop";

	int j = 0; //frame counter
	while (true)
	{
		capture >> frame; //grab frame from video capture
		j++;

		if (!frame.empty())
		{
			face = detectAndDisplay(frame);

			if (face == Rect(Point(0, 0), Point(0, 0))) //if no face detected
				face = prvface;

			if (j % 5 == 0) //set command freq here
			{
				cmd = trackIt(face, frameWidth, frameHeight, frameSize);

				if (cmd != prvcmd)
				{
					cout << cmd << endl;
					telnet_client.write(cmd);
					telnet_client.write('\r');
				}
				prvcmd = cmd;
			}
			prvface = face;
		}
		else
		{
			printf("Error capturing frame");
			break;
		}
		if (27 == char(waitKey(10))) break; //wait for ESC key to exit
	}
	//telnet cleanup
	#ifdef POSIX
		tcsetattr(0, TCSANOW, &stored_settings);
	#endif

	return 0;
}

//detect faces and return their rectangle
Rect detectAndDisplay(Mat frame)
{
	vector<Rect> faces;
	Rect faceRect;
	Point botLeft;
	Point topRight;
	Mat frame_gray;

	cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
	//params: 1-frame 2-vector 3-scale 4-minNeighbors 5-flags 6-minsize 7-maxsize
	cascade.detectMultiScale(frame_gray, faces, 1.2, 3, CV_HAAR_FIND_BIGGEST_OBJECT, Size(100, 100));

	//iterate through all detected faces and draw rectangles on frame
	for (int i = 0; i < faces.size(); i++)
	{
		botLeft = Point(faces[i].x, faces[i].y);
		topRight = Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height);
		faceRect = Rect(botLeft, topRight);
		rectangle(frame, botLeft, topRight, Scalar(0, 255, 0), 2, 8, 0);
	}
	imshow("Automated Lecture Tracking", frame);

	return faceRect;
}

//returns command to send to camera
String trackIt(Rect face, double frameWidth, double frameHeight, double frameSize)
{
	Point center = Point(face.x + face.width / 2, face.y + face.height / 2);
	int faceSize = (face.width * face.height);
	int x_weight = (int)abs(200 * (center.x / frameWidth - 0.5));
	int y_weight = (int)abs(200 * (center.y / frameWidth - 0.5));
	double lowerLimit = 0.35;
	double upperLimit = 0.65;

	if ((center.x > frameWidth*lowerLimit && center.x < frameWidth*upperLimit) || center.x == 0)
		x_weight = 0;

	if ((center.y > frameHeight*lowerLimit && center.y < frameHeight*upperLimit) || center.y == 0)
		y_weight = 0;

	if (x_weight >= y_weight && x_weight > 0)
	{
		if (center.x - frameWidth / 2 > 0)
			return "camera pan right 11";
		else
			return "camera pan left 11";
	}
	else if (x_weight < y_weight && y_weight > 0)
	{
		if (center.y - frameHeight / 2 > 0)
			return "camera tilt down 8";
		else
			return "camera tilt up 8";
	}
	else if (x_weight == 0 && y_weight == 0)
	{
		if (abs(center.x - frameWidth / 2) > abs(center.y - frameHeight / 2))
			return "camera tilt stop";
		else
			return "camera pan stop";
	}
}