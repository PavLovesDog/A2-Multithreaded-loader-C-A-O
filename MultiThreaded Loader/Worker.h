#pragma once
#include <vector>
#include <string>
#include "Threadpool.h"
#include "resource.h"

//extern vector<wstring> g_vecImageFileNames; // reference for IMAGE
//extern vector<wstring> g_vecSoundFileNames; // reference for SOUND

class ITask {
public:
	ITask() {};

	void push_image(LPCWSTR _wsFileToOpen)
	{
		// push images?
		g_vecImageFileNames.push_back(_wsFileToOpen);
	}

	void push_sound()
	{
		// push sound ?
	}

	void display_image()
	{
		// display image in window
		// if displaying multiple images, compensate space
	}

	void play_sound()
	{
		//play sound
	}

private:

};