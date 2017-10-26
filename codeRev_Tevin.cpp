VideoCapture capture(0);

	if (!capture.isOpened()){ printf("Error loading video capture"); return 1; }
	if (!cascade.load(cascade_name)){ printf("Error loading cascade"); return 1; }

	double frameWidth = capture.get(CAP_PROP_FRAME_WIDTH);
	double frameHeight = capture.get(CAP_PROP_FRAME_HEIGHT);
	double frameSize = frameWidth * frameHeight;

	Rect face;
	Rect prv_face = Rect(Point(0, 0), Point(0, 0));
	string cmd;
	string prv_cmd = "camera pan stop";

	int j = 0; //frame counter
	while (true)
	{
		capture >> frame; //grab frame from video capture
		j++;

		if (!frame.empty())
		{
			face = detectAndDisplay(frame);

			if (face == Rect(Point(0, 0), Point(0, 0))) //if no face detected
				face = prv_face;

			if (j % 5 == 0) //set command freq here
			{
				cmd = trackIt(face, frameWidth, frameHeight, frameSize);

				if (cmd != prv_cmd)
				{
					cout << cmd << endl;
					telnet_client.write(cmd);
					telnet_client.write('\r');
				}
				prv_cmd = cmd;
			}
			prv_face = face;
		}
		else
		{
			printf("Error capturing frame");
			break;
		}
		if (27 == char(waitKey(10))) break; //wait for ESC key to exit
	}
