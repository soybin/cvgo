#include "vaaac/vaaac.h"
#include "memory.h"

#include <thread>

struct vec3f {
	float x, y, z;

	vec3f& operator+(vec3f arg) {
		x += arg.x;
		y += arg.y;
		z += arg.z;
		return *this;
	}

	vec3f& operator-(vec3f arg) {
		x -= arg.x;
		y -= arg.y;
		z -= arg.z;
		return *this;
	}

	vec3f& operator*(float arg) {
		x *= arg;
		y *= arg;
		z *= arg;
		return *this;
	}

	vec3f& operator/(float arg) {
		x /= arg;
		y /= arg;
		z /= arg;
		return *this;
	}
};

int main(int argc, char* argv[]) {
	// game memory manipulation
	memory* mem = new memory();
	std::cout << "[~] looking for csgo.exe process." << std::endl;
	for (; !mem->init(L"csgo.exe", { L"engine.dll", L"client.dll" }); ) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	std::cout << "[+] process csgo.exe has been found." << std::endl;

	// define pattern structures
	patternStruct localPlayerPattern = { "\x8D\x34\x85\x00\x00\x00\x00\x89\x15\x00\x00\x00\x00\x8B\x41\x08\x8B\x48\x04\x83\xF9\xFF", "xxx????xx????xxxxxxxxx", L"client.dll", true, 4, { 3 } };
	patternStruct forceAttackPattern = { "\x89\x0D\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x8B\xF2\x8B\xC1\x83\xCE\x04", "xx????xx????xxxxxxx", L"client.dll", true, 0, { 2 } };
	patternStruct dwClientStatePattern = { "\xA1\x00\x00\x00\x00\x33\xD2\x6A\x00\x6A\x00\x33\xC9\x89\xB0", "x????xxxxxxxxxx", L"engine.dll", true, 0, { 1 } };
	patternStruct viewAnglesPattern = { "\xF3\x0F\x11\x80\x00\x00\x00\x00\xD9\x46\x04\xD9\x05", "xxxx????xxxxx", L"engine.dll", false, 0, { 4 } };

	// get necessary offsets
	unsigned long localPlayerOffset = mem->getOffset(localPlayerPattern);
	unsigned long forceAttackOffset = mem->getOffset(forceAttackPattern);
	unsigned long dwClientStateOffset = mem->getOffset(dwClientStatePattern);
	unsigned long viewAnglesOffset = mem->getOffset(viewAnglesPattern);

	// add module base addresses for some of the dynamic offsets
	unsigned long localPlayer = mem->read<unsigned long>(mem->modules[L"client.dll"].first + localPlayerOffset);
	unsigned long dwClientState = mem->read<unsigned long>(mem->modules[L"engine.dll"].first + dwClientStateOffset);

	/*
	 * initialize the very awesome
	 * arm angle calculation library!
	 */
	vaaac* v = new vaaac();

	/*
	 * skin tone calibration is required
	 * if you don't hardcode the values
	 * yourself
	 */
	v->calibrateSkinTone();

	// get initial angle view, so that
	// it doesn't reset to zero
	vec3f initialAngles = mem->read<vec3f>(dwClientState + viewAnglesOffset);
	vec3f curAngle = initialAngles;
	float smoothness = 5.0;
	for (; v->isOk(); ) {
		// new frame
		v->update();

		// change angles
		double angleX = initialAngles.y + v->getXAngle();
		double angleY = v->getYAngle();
		// player's skin hasn't been found
		if (v->isDetected()) {
			vec3f adjustedCurAngle = { angleY, angleX, initialAngles.z };
			if (adjustedCurAngle.y > 180.0) adjustedCurAngle.y -= 360.0;
			else if (adjustedCurAngle.y < -180.0) adjustedCurAngle.y += 360.0;
			if (adjustedCurAngle.x < 90.0 && adjustedCurAngle.x > -90.0) {
				mem->write<vec3f>(dwClientState + viewAnglesOffset, adjustedCurAngle);
			}
		} else {
			mem->write<vec3f>(dwClientState + viewAnglesOffset, initialAngles);
		}

		// check if triggered
		if (v->isTriggered()) {
			// shoot
			mem->write<int>(mem->modules[L"client.dll"].first + forceAttackOffset, 5);
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			mem->write<int>(mem->modules[L"client.dll"].first + forceAttackOffset, 4);
		}

		// user input
		int key = cv::waitKey(1);
		if (key == 27) {
			break;
		}
	}
	delete v;
	delete mem;
	return 0;
}