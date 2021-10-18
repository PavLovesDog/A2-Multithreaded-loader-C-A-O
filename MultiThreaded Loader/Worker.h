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

	void push_image()
	{
		// push image?
	}

	void push_sound()
	{
		// push sound ?
	}

private:

};