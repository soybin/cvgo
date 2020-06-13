/*
 * MIT License
 * Copyright (c) 2020 Pablo Peñarroja
 */

#include <algorithm>

#include "vaaac.h"

vaaac::vaaac() {
	// webcam initialization
	videoCapture = cv::VideoCapture(0);
	videoCapture.set(cv::CAP_PROP_SETTINGS, 1);
	// check if it's alright
	ok = videoCapture.isOpened();
	if (!ok) {
		ok = false;
		return;
	}
	// resolution (1:1 aspect ratio)
	width = videoCapture.get(cv::CAP_PROP_FRAME_WIDTH);
	height = videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT);
	res = std::min(width, height);
	halfRes = res / 2;
	// make the viewport a centered square
	int addX = 0, addY = 0;
	if (width > height) {
		addX = (width - height) / 2;
	}
	else if (height > width) {
		addY = (height - width) / 2;
	}
	// limit camera resolution ratio to 1:1
	frameBounds = cv::Rect(addX, addY, res, res);
	// determine reticle view area
	int reticlePos = halfRes - RETICLE_SIZE / 2;
	reticleBounds = cv::Rect(reticlePos, reticlePos, RETICLE_SIZE, RETICLE_SIZE);
	// determine no aim zone
	int noAimAreaPos = halfRes - NO_AIM_AREA_SIZE / 2;
	noAimAreaBounds = cv::Rect(noAimAreaPos, noAimAreaPos, NO_AIM_AREA_SIZE, NO_AIM_AREA_SIZE);
	// fill up bfs offsets array
	bfsOffsets.clear();
	for (int i = -1; i < 2; ++i) {
		for (int j = -1; j < 2; ++j) {
			bfsOffsets.push_back(std::make_pair<int, int>(i * BFS_SAMPLE_SIZE, j * BFS_SAMPLE_SIZE));
		}
	}
	// precompute trigger system constants
	TRIGGER_MINIMUM_DISTANCE_PIXELS = TRIGGER_MINIMUM_DISTANCE * res / 100.0;
	TRIGGER_MAXIMUM_DISTANCE_PIXELS = TRIGGER_MAXIMUM_DISTANCE * res / 100.0;
	TRIGGER_ALLOWED_Y_DEVIATION_PIXELS = TRIGGER_ALLOWED_Y_DEVIATION * res / 100.0;
	TRIGGER_ALLOWED_X_DEVIATION_PIXELS = TRIGGER_ALLOWED_X_DEVIATION * res / 100.0;
	// clear y variance deque
	increment = true;
	yxDeltaSize = 0;
	yxDelta.clear();
}

vaaac::~vaaac() {
}

void vaaac::calibrateSkinTone() {
	if (ok & 1) {
		int xCoord = res / 2 - SAMPLE_AREA_WIDTH / 2;
		int yCoord = res / 2 - SAMPLE_AREA_HEIGHT / 2;
		int rectSizeX = std::min(SAMPLE_AREA_WIDTH, halfRes);
		int rectSizeY = std::min(SAMPLE_AREA_HEIGHT, halfRes);
		cv::Rect area(xCoord, yCoord, rectSizeX, rectSizeY);
		for (;;) {
			videoCapture >> frame;
			frame = frame(frameBounds);
			if (RENDER_SAMPLE_TEXT) {
				cv::putText(
					frame,
					"fill the area with your skin.",
					cv::Point(10, area.y - 60),
					cv::FONT_HERSHEY_DUPLEX,
					1.0,
					cv::Scalar(255, 255, 255),
					1);
				cv::putText(
					frame,
					"then press any key.",
					cv::Point(10, area.y - 20),
					cv::FONT_HERSHEY_DUPLEX,
					1.0,
					cv::Scalar(255, 255, 255),
					1);
			}
			/*
			 * check if user is done before
			 * drawing rectangle because,
			 * otherwise, the rectangle gets
			 * computed as the skin tone mean
			 */
			int key = cv::waitKey(1);
			if (key != -1) {
				break;
			}
			// now draw rectangle and draw
			if (RENDER_TO_WINDOW) {
				cv::rectangle(frame, area, cv::Scalar(255, 255, 255), 2);
				cv::imshow("calibrateSkinTone", frame);
			}
		}
		if (RENDER_TO_WINDOW) {
			cv::destroyWindow("calibrateSkinTone");
		}
		cv::Mat hsv;
		cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
		cv::Mat sample(hsv, area);
		cv::Scalar mean = cv::mean(sample);
		hLow = mean[0] - MASK_LOW_TOLERANCE;
		hHigh = mean[0] + MASK_HIGH_TOLERANCE;
		sLow = mean[1] - MASK_LOW_TOLERANCE;
		sHigh = mean[1] + MASK_HIGH_TOLERANCE;
		vLow = 0;
		vHigh = 255;
		ok = 2;
	}
}

void vaaac::update() {

	//                                    //
	//-- i m a g e  p r o c e s s i n g --//
	//                                    //

	// always false before processing
	detected = false;
	triggered = false;
	// get current frame
	videoCapture >> frame;
	// reshape
	frame = frame(frameBounds);
	// to hsv
	cv::cvtColor(frame, mask, cv::COLOR_BGR2HSV);
	// binarization
	cv::inRange(mask, cv::Scalar(hLow, sLow, vLow), cv::Scalar(hHigh, sHigh, vHigh), mask);
	// noise reduction
	cv::Mat structuringElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, { 3, 3 });
	cv::morphologyEx(mask, mask, cv::MORPH_OPEN, structuringElement);
	cv::dilate(mask, mask, cv::Mat(), { -1, -1 }, 1);
	/*
	 * check existance of object within
	 * the reticle area's
	 */
	int xMin = reticleBounds.x;
	int yMin = reticleBounds.y;
	int xMax = reticleBounds.x + reticleBounds.width;
	int yMax = reticleBounds.y + reticleBounds.height;
	int xAim = halfRes;
	int yAim = halfRes;
	xAngle = -100.0;
	yAngle = -100.0;
	if (cv::mean(mask(reticleBounds))[0] > 0) {
		detected = true;
		std::vector<std::vector<bool>> visited(res + 1, std::vector<bool>(res + 1, false));
		std::queue<std::pair<int, int>> q;
		for (int i = halfRes - RETICLE_SIZE / 2; i <= halfRes + RETICLE_SIZE / 2; i += BFS_SAMPLE_SIZE) {
			for (int j = halfRes - RETICLE_SIZE / 2; j <= halfRes + RETICLE_SIZE / 2; j += BFS_SAMPLE_SIZE) {
				for (auto& offset : bfsOffsets) {
					if (offset.first < 0 || offset.second < 0) {
						continue;
					}
					int x = i + offset.first, y = j + offset.second;
					q.push({ x, y });
				}
			}
		}
		for (; !q.empty(); ) {
			std::pair<int, int> xy = q.front();
			int x = xy.first, y = xy.second;
			q.pop();
			if (x < 0 || y < 0 || x + BFS_SAMPLE_SIZE > res || y + BFS_SAMPLE_SIZE > res || visited[x][y]) continue;
			visited[x][y] = true;
			if (cv::mean(mask(cv::Rect(x, y, BFS_SAMPLE_SIZE, BFS_SAMPLE_SIZE)))[0] == 0) continue;
			/*
			 * update furthermost point coordinates.
			 * works because the bfs algorithm always
			 * visits the furthermost element in the
			 * last place
			 */
			xAim = x;
			yAim = y;
			// update found object area bounds
			xMin = std::min(xMin, x);
			yMin = std::min(yMin, y);
			xMax = std::max(xMax, x);
			yMax = std::max(yMax, y);
			// add neighbors to queue
			for (auto& offset : bfsOffsets) {
				q.push(std::make_pair<int, int>(x + offset.first, y + offset.second));
			}
		}
		/*
		 * check if aim point falls within
		 * the no aim area boundaries.
		 * set angles to zero if that's the
		 * case
		 */
		if (noAimAreaBounds.contains(cv::Point(xAim, yAim))) {
			yAim = halfRes;
			xAim = halfRes;
		}
		/*
		 * check if there's a clear peak in
		 * the y variance
		 */
		if (!yxDeltaSize) {
			yxDelta.push_back({ yAim, xAim });
			yxDeltaSize = 1;
		} else if (increment) {
			if (yAim <= yxDelta[yxDeltaSize - 1].first) {
				++yxDeltaSize;
				yxDelta.push_back({ yAim, xAim });
			} else {
				int yDelta = yxDelta[0].first - yxDelta[yxDeltaSize - 1].first;
				if (yDelta >= TRIGGER_MINIMUM_DISTANCE_PIXELS && yDelta <= TRIGGER_MAXIMUM_DISTANCE_PIXELS) {
					increment = false;
				} else {
					yxDeltaSize = 0;
					yxDelta.clear();
				}
			}
		} else {
			if (yAim >= yxDelta[yxDeltaSize - 1].first) {
				yxDelta.push_back({ yAim, xAim });
				++yxDeltaSize;
			} else {
				std::pair<int, int> left = yxDelta[0];
				std::pair<int, int> right = yxDelta[yxDeltaSize - 1];
				bool ok = true;
				ok &= std::abs(left.first - right.first) <= TRIGGER_ALLOWED_Y_DEVIATION_PIXELS;
				ok &= std::abs(left.second - right.second) <= TRIGGER_ALLOWED_X_DEVIATION_PIXELS;
				if (ok) {
					triggered = true;
					yAim = left.first;
					xAim = left.second;
				}
				increment = true;
				yxDelta.clear();
			}
		}
		// make angles
		yAngle = -(double)(halfRes - yAim) / halfRes * 90.0;
		xAngle = -(double)(halfRes - xAim) / halfRes * 90.0;
		yAngleSmooth += (yAngle - yAngleSmooth) / (double)(AIM_SMOOTHNESS);
		xAngleSmooth += (xAngle - xAngleSmooth) / (double)(AIM_SMOOTHNESS);
		/*Fyx
		 * cut out everything outside of the
		 * object boundaries
		 */
		mask(cv::Rect(0, 0, xMin, res)).setTo(cv::Scalar(0));
		mask(cv::Rect(xMin, 0, res - xMin, yMin)).setTo(cv::Scalar(0));
		mask(cv::Rect(xMin, yMax, res - xMin, height - yMax)).setTo(cv::Scalar(0));
		mask(cv::Rect(xMax, yMin, res - xMax, yMax - yMin)).setTo(cv::Scalar(0));
	}

	//                                   //
	//-------- r e n d e r i n g --------//
	//                                   //

	if (RENDER_TO_FRAME) {
		/*
		 * draw rectangle indicating either
		 * the reticle bounds or
		 * the found object boundaries'
		 */
		cv::rectangle(mask, cv::Rect(xMin, yMin, xMax - xMin, yMax - yMin), cv::Scalar(255, 255, 255), 2);
		/*
		 * draw circle indicating furthermost
		 * point from origin
		 */
		cv::circle(frame, cv::Point(xAim, yAim), 5, cv::Scalar(255, 0, 255), 2);
		// draw current angles
		cv::putText(
			frame,
			"x angle: " + std::to_string(xAngle),
			cv::Point(0, 20),
			cv::FONT_HERSHEY_DUPLEX,
			0.5,
			cv::Scalar(255, 255, 255),
			1);
		cv::putText(
			frame,
			"y angle: " + std::to_string(yAngle),
			cv::Point(0, 40),
			cv::FONT_HERSHEY_DUPLEX,
			0.5,
			cv::Scalar(255, 255, 255),
			1);
		// convert mask to three channel
		cv::cvtColor(mask, mask, cv::COLOR_GRAY2RGB);
		// mix frame with mask
		cv::addWeighted(mask, 0.5, frame, 1.0, 0.0, frame);
		// present final image
		if (RENDER_TO_WINDOW) {
			cv::imshow("update", frame);
		}
	}
}