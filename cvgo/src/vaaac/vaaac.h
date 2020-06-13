/*
 * MIT License
 * Copyright (c) 2020 Pablo Peñarroja
 */

 /*
  * the following constants should be
  * altered in order to fit the user's
  * needs.
  * they're set to standard values
  * by default, and changing them may
  * dramatically alter the library's
  * behaviour
  */

  //                                   //
  //-------- c o n s t a n t s --------//
  //                                   //

  /*
   * this indicates whether to render
   * the processed image to a matrix
   * 'frame'.
   * this matrix can be retrieved using
   * the 'getFrame()' method declared
   * bellow
   */
const bool RENDER_TO_FRAME = true;

/*
 * this indicates whether to create a
 * separate window and render the
 * current frame to that window.
 * it requires the 'RENDER_TO_FRAME'
 * constant to be true
 */
const bool RENDER_TO_WINDOW = true;

/*
 * this indicates whether to render
 * a small text explaining to the user
 * what to do
 */
const bool RENDER_SAMPLE_TEXT = true;

/*
 * width and height of the skin color
 * sampling rectangle
 */
const int SAMPLE_AREA_WIDTH = 30;
const int SAMPLE_AREA_HEIGHT = 30;

/*
 * this is the tolerance applied to
 * the lower and higher bounds of the
 * HSV skin tone color sampled from
 * the user.
 * low values limit the range, thus
 *
 */
const int MASK_LOW_TOLERANCE = 50;
const int MASK_HIGH_TOLERANCE = 25;

/*
 * width and height of the reticle:
 * the area centered at the middle of
 * the screen where the program will
 * look for the user's skin.
 * in case of finding it, the object
 * union find algorithm will start.
 */
const int RETICLE_SIZE = 40;

/*
 * if an object is detected, but the
 * aim point is located within this
 * area, the aim point will be instantly
 * set to zero.
 * the reason for this being that it's
 * very hard to distinguish the accurate
 * location of the aim point when the
 * user is pointing straight at the
 * camera
 */
const int NO_AIM_AREA_SIZE = 80;

/*
 * the bfs sample size represents the
 * square root of the area to scan for
 * each node of the bfs algorithm.
 * the lower this value, the more
 * accurate but the more noise, and the
 * more cpu usage required as well
 */
const int BFS_SAMPLE_SIZE = 2;

/*
 * smoothness is used to get rid of some
 * of the noise in the calculations.
 * the higher this number, the more
 * smoothness, less noise, but less
 * responsiveness as well
 */
const int AIM_SMOOTHNESS = 4;

/*
 * this is the minumum distance that
 * the aim point should travel upwards
 * in order to be considered a trigger
 * action.
 * it's represented as a percentage
 * of the screen real estate
 */
const double TRIGGER_MINIMUM_DISTANCE = 3.0;

/*
 * this is the maximum distance that
 * the aim point is allowed to  travel 
 * upwards in order to be considered a
 * trigger action.
 * it's represented as a percentage
 * of the screen real estate
 */
const double TRIGGER_MAXIMUM_DISTANCE = 15.0;

/*
 * consider y1 as the origin y value
 * for a trigger action, and y2 as the
 * final y value, then:
 * this value represents the absolute
 * maximum allowed deviation from y1
 * to y2 in order for the action to
 * be valid.
 * it's represented as a percentage of
 * the screen real estate
 */
const double TRIGGER_ALLOWED_Y_DEVIATION = 3.0;

/*
 * same as 'TRIGGER_ALLOWED_Y_DEVIATION'
 * but for the x-axis
 */
const double TRIGGER_ALLOWED_X_DEVIATION = 2.0;

//                                 //
//--------  v  a  a  a  c  --------//
//                                 //

#pragma once

#include <vector>
#include <utility>
#include <queue>
#include <deque>

#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

class vaaac {
private:
	// is everything working
	int ok;

	// player detected
	bool detected;

	// resolution (square)
	int width;
	int height;
	int res;
	int halfRes;

	// computer vision vars
	cv::VideoCapture videoCapture;
	cv::Rect frameBounds;
	cv::Rect reticleBounds;
	cv::Rect noAimAreaBounds;
	cv::Mat frame;
	cv::Mat mask;

	// skin tone hsv color bounds
	int hLow;
	int hHigh;
	int sLow;
	int sHigh;
	int vLow;
	int vHigh;

	// breadth first search system
	std::vector<std::pair<int, int>> bfsOffsets;

	// aimed at point location
	double xAngle;
	double yAngle;

	// stepped point location
	double xAngleSmooth;
	double yAngleSmooth;

	// trigger system
	bool triggered;
	bool increment;
	int yxDeltaSize;
	int TRIGGER_MINIMUM_DISTANCE_PIXELS;
	int TRIGGER_MAXIMUM_DISTANCE_PIXELS;
	int TRIGGER_ALLOWED_Y_DEVIATION_PIXELS;
	int TRIGGER_ALLOWED_X_DEVIATION_PIXELS;
	std::vector<std::pair<int, int>> yxDelta;

public:
	vaaac();
	~vaaac();

	inline bool isOk() {
		return ok == 2;
	}

	inline bool isDetected() {
		return detected;
	}

	inline bool isTriggered() {
		return triggered;
	}

	inline double getXAngle() {
		return xAngleSmooth;
	}

	inline double getYAngle() {
		return yAngleSmooth;
	}

	inline cv::Mat getFrame() {
		return frame;
	}

	void calibrateSkinTone();
	void update();
};