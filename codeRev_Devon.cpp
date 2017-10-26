//returns command to send to camera
String trackIt(Rect face, double frameWidth, double frameHeight, double frameSize)
{
	Point center = Point(face.x + face.width / 2, face.y + face.height / 2);
	int faceSize = (face.width * face.height);
	int x_weight = (int)abs(200 * (center.x / frameWidth - 0.5));
	int y_weight = (int)abs(200 * (center.y / frameWidth - 0.5));
	double lowerLimit = 0.4;
	double upperLimit = 0.6;

	if ((center.x > frameWidth*lowerLimit && center.x < frameWidth*upperLimit) || center.x == 0)
		x_weight = 0;

	if ((center.y > frameHeight*lowerLimit && center.y < frameHeight*upperLimit) || center.y == 0) 
		y_weight = 0;

	if (x_weight >= y_weight && x_weight > 0)
	{
		if (center.x - frameWidth / 2 > 0)
			return "camera pan right";
		else
			return "camera pan left";
	}
	else if (x_weight < y_weight && y_weight > 0)
	{
		if (center.y - frameHeight / 2 > 0)
			return "camera tilt down";
		else
			return "camera tilt up";
	}
	else if (x_weight == 0 && y_weight == 0)
	{
		if (abs(center.x - frameWidth / 2) > abs(center.y - frameHeight / 2))
			return "camera tilt stop";
		else
			return "camera pan stop";
	}
}
