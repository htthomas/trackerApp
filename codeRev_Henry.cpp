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
