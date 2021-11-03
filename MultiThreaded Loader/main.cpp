
#include <Windows.h>
#include <mmsystem.h> // multimedia system, for sounds
#include <functional> // for ref()
#include <algorithm> // for joining threads function mem_fn
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono> // for timing resources
#include <sstream> // for float to LPCWSTR conversions
#include <fstream> // for file reading
//#include <iostream>
#include "resource.h"

#define WINDOW_CLASS_NAME L"MultiThreaded Loader Tool"
const unsigned int _kuiWINDOWWIDTH = 600;
const unsigned int _kuiWINDOWHEIGHT = 800;
#define MAX_FILES_TO_OPEN 50
#define MAX_CHARACTERS_IN_FILENAME 25

using std::thread;
using std::vector;
using std::wstring;
using std::mutex;
using std::ref;
using std::chrono::steady_clock;
using std::chrono::duration;

//Global Variables
//float maxThreads;// = thread::hardware_concurrency(); // Create varibale for total threads
vector<wstring> g_vecImageFileNames;
vector<wstring> g_vecSoundFileNames;
HINSTANCE g_hInstance; // creating an instance of window handle
bool g_bIsFileLoaded = false; 
bool imageLoaded = false;
bool soundLoaded = false;
vector<thread> myThreads;
vector<HBITMAP> bitMaps;
vector<float> imageTimes;
vector<float> soundTimes;

vector<LPCWSTR> sounds;

int xc = 0; // 100; // x-coordinate for top left point of image 
int yc = 0; // 100; // y-coordinate for top left point, where image drawn from

HBITMAP hBitMap;
mutex gLock; // window creation lock
mutex sLock;

/* Function to set the max threads for the application use
   this is determine via an extrenal file, located within 
   the bmp or wav file folders, the file which is read depends
   on the filetype to be loaded
   Returns the maxThreads as an int */
int SetThreads(HWND _hwnd)
{
	int threads = 0;

	std::ifstream inputFile;
	inputFile.open("threadsToUse.txt"); // .txt file MUST be in same directory as .bmp images loaded... why?? no idea
										// .txt document Located withing sln folder / bmp folder
	if (!inputFile) // check file validity
	{
		MessageBox(_hwnd, L"Failed to open file.", L"ERROR", MB_OK);
		return 0;
	}
	// Get data
	if (inputFile.is_open())
	{
			inputFile >> threads;
	}
	else
	{
		MessageBox(_hwnd, L"Unable to open file.", L"ERROR", MB_OK);
		return 0;
	}

	inputFile.close();

	return threads;
}

/* a function to check for errors when loading images to memory
   will display a message to window if errors occur */
void ErrorCheck(HWND _hwnd, int index, DWORD error)
{
	// check if loaded correctly
	if (bitMaps[index] == NULL)
	{
		if (error == 0) // operation performed successfully
		{
			// do nothing, we're good.
		}
		if (error == 2) // Cannot Locate
		{
			MessageBox(_hwnd, L"The system cannot find the file specified", L"Filepath Invalid", MB_ICONWARNING);
		}
		else if (error == 4) // cannot open file
		{
			MessageBox(_hwnd, L"BitMap is Null", L"Image Loaded Failed", MB_ICONWARNING);
		}
		else if (error == 6) // invalid handle
		{
			MessageBox(_hwnd, L"Handle is Invalid", L"Window Handle Error", MB_ICONWARNING);
		}
	}
}

/* simple function to play a wav file loaded to the program */
void PlayWAV(int index)
{
	// playing sound using the windows multimedia library source: https://www.youtube.com/watch?v=2Fqh-8XqK0M
	sLock.lock();
	PlaySound((LPCWSTR)g_vecSoundFileNames[index].c_str(), NULL, SND_SYNC);
	sLock.unlock();
}

/* a function which iterates through multiple sounds to load/play
   this variation is supposed to be threaded when large amounts of
   sound files are loaded */
void PlayWAVThreaded(int index, int soundsPerLoad)
{
	sLock.lock();
	for (int i = 0; i < soundsPerLoad; i++)
	{
		PlaySound((LPCWSTR)g_vecSoundFileNames[(std::uint64_t)index + i].c_str(), NULL, SND_SYNC);
	}
	sLock.unlock();
}

/* A function to load a bitmap file into memory via a HBITMAP
   file */
void loadPic(int index, HWND _hwnd)
{
	gLock.lock();
	bitMaps[index] = (HBITMAP)LoadImageW(NULL, (LPCWSTR)g_vecImageFileNames[index].c_str(), IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE);
	DWORD error = GetLastError(); // note any errors
	gLock.unlock();

	// check for load errors & display
	ErrorCheck(_hwnd, index, error);
}

/* Function which creates a window to paint a single image to within the 
   window handle*/
void PaintImageNow(HWND _hwnd, int ImageNo)
{
	gLock.lock();
	_hwnd = CreateWindow(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, xc, yc, 0, 0, _hwnd, NULL, NULL, NULL);
	gLock.unlock();

	SendMessageW(_hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitMaps[ImageNo]);
}

/* A function t handle the threading and loading of multiple image files*/
void ThreadLoadImage(HWND _hwnd, int loadsPerThread, int imageIndex)
{
	DWORD error;

	gLock.lock();
		for (int i = 0; i < loadsPerThread; i++)
		{
			/* cast the 4byte variable imageIndex to 8byte to avoid overflow, recommended by intellisense */
			bitMaps[i + (std::uint64_t)imageIndex] = (HBITMAP)LoadImageW(NULL, (LPCWSTR)g_vecImageFileNames[i + (std::uint64_t)imageIndex].c_str(), IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE);
			error = GetLastError(); // note any errors
			ErrorCheck(_hwnd, (i + (std::uint64_t)imageIndex), error);
		}
	gLock.unlock();
}

// FAILED ATTEMPT TO THREAD PAINT
void ThreadPaintImage(HWND wnd, int imagesPerThread, int imageBlock)
{
	int xPos = 0;
	int yPos = 0;

	gLock.lock();

	// loop through the images needed to paint
	for (int i = 0; i < imagesPerThread; i++)
	{
		//create window THIS MAY NEED TO NOT BE ON THREAD!? ...Then how to implement?
		wnd = CreateWindow(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, xc + xPos, yc + yPos, 0, 0, wnd, NULL, NULL, NULL); //TODO thread stalls on this??

		// send message to paint the image
		SendMessageW(wnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitMaps[i + (std::uint64_t)imageBlock]); // cast the 4byte to 8byte to avoid overflow, recommended by intellisense 

		// increment x - y coordinates so images do not display over one another
		xPos += 100; // move next image too be painted along by 100 pixels
		if ((xc + xPos) >= _kuiWINDOWWIDTH) // if the culmination of x values greater than window size
		{
			yPos += 100; // drop images to next line

			//vLock.lock();
			xc = 0;  // reset x-coordinate
			//vLock.unlock();
		}
	}
	gLock.unlock();
}

//NOMANS CONTROLLER, cannot multi thread
void Controller_Multi(HWND wnd, int ImageNo)
{
	//gLock.lock();
	
	HBITMAP imageFile = bitMaps[ImageNo];
	
	int xPos = (ImageNo * 100);
	int yPos = 0;

	if (yPos > 0)
	{
		xPos = ((ImageNo - 8) * 100);
	}
	else
	{
		xPos = ImageNo * 100;
	}

	if (xPos >= _kuiWINDOWWIDTH)
	{
		yPos += 100;
		xPos = 0;
	}

	wnd = CreateWindow(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, xPos, yPos, 0, 0, wnd, NULL, NULL, NULL);
	//gLock.unlock();

	SendMessageW(wnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)imageFile);
}

bool ChooseImageFilesToLoad(HWND _hwnd) // passing in the windows handle
{
	OPENFILENAME ofn;
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1];
	wchar_t _wstempFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME]; //Assuming that the filename is not more than 20 characters
	wchar_t _wsFileToOpen[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	ZeroMemory(wsFileNames, sizeof(wsFileNames));
	ZeroMemory(_wsPathName, sizeof(_wsPathName));
	ZeroMemory(_wstempFile, sizeof(_wstempFile));

	//Fill out the fields of the structure
	ofn.lStructSize = sizeof(OPENFILENAME); //The length, in bytes, of the structure
	ofn.hwndOwner = _hwnd; // A handle to the window that owns the dialog box
	ofn.lpstrFile = wsFileNames; // The file name used to initialize the 'File Name' edit control, this is what our files loaded in will be handled through
	ofn.nMaxFile = MAX_FILES_TO_OPEN * 20 + MAX_PATH;  // The size, in characters of the buffer pointed to by lpstrFile. 
	                                                   // The buffer must be atleast 256(MAX_PATH) characters long; otherwise GetOpenFileName and 
													   // GetSaveFileName functions return False
													   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
													   // use the contents of wsFileNames to initialize itself.
	ofn.lpstrFile[0] = '\0'; // set first file buffer to NULL terminated string
	ofn.lpstrFilter = L"Bitmap Images(.bmp)\0*.bmp\0"; //Filter for bitmap images, defines the file type to load
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT; // set flags of the file function

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//First way: just work with wide char arrays
		wcsncpy_s(_wsPathName, wsFileNames, ofn.nFileOffset);
		int i = ofn.nFileOffset;
		int j = 0;

		while (true)
		{
			if (*(wsFileNames + i) == '\0')
			{
				_wstempFile[j] = *(wsFileNames + i);
				wcscpy_s(_wsFileToOpen, _wsPathName);
				wcscat_s(_wsFileToOpen, L"\\");
				wcscat_s(_wsFileToOpen, _wstempFile);
				g_vecImageFileNames.push_back(_wsFileToOpen);

				j = 0;
			}
			else
			{
				_wstempFile[j] = *(wsFileNames + i);
				j++;
			}

			if (*(wsFileNames + i) == '\0' && *(wsFileNames + i + 1) == '\0')
			{
				break;
			}
			else
			{
				i++;
			}

		}

		g_bIsFileLoaded = true;
		return true;
	}
	else // user pressed the cancel button or closed the dialog box or an error occured
	{
		return false;
	}

}

bool ChooseSoundFilesToLoad(HWND _hwnd)
{
	OPENFILENAME ofn;
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1];
	wchar_t _wstempFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME]; //Assuming that the filename is not more than 20 characters
	wchar_t _wsFileToOpen[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	ZeroMemory(wsFileNames, sizeof(wsFileNames));
	ZeroMemory(_wsPathName, sizeof(_wsPathName));
	ZeroMemory(_wstempFile, sizeof(_wstempFile));

	//Fill out the fields of the structure
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = _hwnd;
	ofn.lpstrFile = wsFileNames;
	ofn.nMaxFile = MAX_FILES_TO_OPEN * 20 + MAX_PATH;  //The size, in charactesr of the buffer pointed to by lpstrFile. The buffer must be atleast 256(MAX_PATH) characters long; otherwise GetOpenFileName and 
													   //GetSaveFileName functions return False
													   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
													   // use the contents of wsFileNames to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = L"Wave Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0"; //Filter for wav files
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//Second way: work with wide strings and a char pointer 

		std::wstring _wstrPathName = ofn.lpstrFile;

		_wstrPathName.resize(ofn.nFileOffset, '\\');

		wchar_t *_pwcharNextFile = &ofn.lpstrFile[ofn.nFileOffset];

		while (*_pwcharNextFile)
		{
			std::wstring _wstrFileName = _wstrPathName + _pwcharNextFile;

			//TODO SOUND LOADING
			myThreads.emplace_back(thread([=]()
			{
				g_vecSoundFileNames.push_back(_wstrFileName);
			}));

			_pwcharNextFile += lstrlenW(_pwcharNextFile) + 1;
		}

		for_each(myThreads.begin(), myThreads.end(), mem_fn(&thread::join)); // join threads
		myThreads.clear(); // clear vector after operations are done
		g_bIsFileLoaded = true;
		return true;
	}
	else // user pressed the cancel button or closed the dialog box or an error occured
	{
		return false;
	}

}

LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _uiMsg, WPARAM _wparam, LPARAM _lparam)
{
	PAINTSTRUCT ps;
	HDC _hWindowDC;

	//RECT rect;
	switch (_uiMsg)
	{
	case WM_KEYDOWN:
	{
		switch (_wparam)
		{
		case VK_ESCAPE:
		{
			MessageBox(_hwnd, L"Okay, bye bye.", L"You're leaving?", MB_OK); //TODO not neces
			SendMessage(_hwnd, WM_CLOSE, 0, 0);
			return(0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_PAINT:
	{
		_hWindowDC = BeginPaint(_hwnd, &ps);

		//TextOut(_hWindowDC, 0, 0, L"Yo, Waddup", 15); // this works, and is constantly painted
		//Rectangle(_hWindowDC, 100, 100, 200, 200); // this works
		RoundRect(_hWindowDC, 100, 300, 400, 400, 20, 20);

		if (g_bIsFileLoaded)
		{
			//TODO THIS WAS AN ATTEMPT TO THREAD PAINTING
			//int loadsPerThread = 0; // initialize variable for large no. images selected
			//
			//	// determine how many files to be loaded per thread
			//if (g_vecImageFileNames.size() > maxThreads)
			//{
			//	loadsPerThread = floor(g_vecImageFileNames.size() / maxThreads);
			//}
			//
			//if (loadsPerThread > 0) // if there are more images than total threads available to load
			//{
			//	//divide work over all available threads
			//	int imageIndex = 0; // create image block index
			//	for (int i = 0; i < maxThreads; i++) // loops to start the threads
			//	{
			//		ThreadPaintImage(_hwnd, loadsPerThread, imageIndex);
			//		//myThreads[i] = thread(ThreadPaintImage, _hwnd, loadsPerThread, imageIndex);
			//		imageIndex += loadsPerThread; // increment index for next thread start
			//
			//		/* BELOW WAS FOR PAINTING - Could use for LOADING? */
			//		xc += 100 * loadsPerThread; // move along by amount of pictures painted in last thread
			//		if (xc >= _kuiWINDOWWIDTH)
			//		{
			//			yc += 100; // drop images to next line
			//			xc = 0; // reset x-coordinate
			//		}
			//	}
			//}
			//else // sufficient threads are available
			//TODO JOIN THIS ELSE TO BELOW CODE FOR PAINTING THREAD - NOT WORKING
			if (imageLoaded)
			{
				InvalidateRect(_hwnd, NULL, true);

				auto startPaintTime = steady_clock::now();
				for (int i = 0; i < bitMaps.size(); i++)
				{
					PaintImageNow(_hwnd, i);
				
					//alter variables for image paint on next loop
					xc += 100; // move next image too be painted along by 100 pixels
					if (xc >= _kuiWINDOWWIDTH)
					{
						yc += 100; // drop images to next line
						xc = 0; // reset x-coordinate
					}
				}
				auto endPaintTime = steady_clock::now();
				duration<double> totalpaintTime = endPaintTime - startPaintTime;
				imageTimes[1] = totalpaintTime.count(); // add paint time to float vector
				imageTimes[2] = imageTimes[0] + imageTimes[1]; // total time
				
				
				// float to LPCWSTR conversion sourced by user Sameer: https://stackoverflow.com/questions/2481787/convert-float-to-lpcwstr-lpwstr
				float paintTime = imageTimes[1];
				std::wstringstream PT;
				PT << paintTime;
				float loadTime = imageTimes[0];
				std::wstringstream LT;
				LT << loadTime;
				float totalTime = imageTimes[2];
				std::wstringstream TT;
				TT << totalTime;
				
				// load time messages
				TextOut(_hWindowDC, 5, 10, L"Load Time Taken: ", 17);
				TextOut(_hWindowDC, 130, 10, LT.str().c_str(), 10);
				TextOut(_hWindowDC, 200, 10, L"milliseconds", 12);
				
				// Paint time messages
				TextOut(_hWindowDC, 5, 50, L"Paint Time Taken: ", 17);
				TextOut(_hWindowDC, 130, 50, PT.str().c_str(), 10);
				TextOut(_hWindowDC, 200, 50, L"milliseconds", 12);
				
				// Total Time Message
				TextOut(_hWindowDC, 5, 100, L"TOTAL TIME: ", 12);
				TextOut(_hWindowDC, 130, 100, TT.str().c_str(), 10);
				TextOut(_hWindowDC, 200, 100, L"milliseconds", 12);
				
				//clear bitmap & string vector for if user chooses to load more images
				bitMaps.clear();
				g_vecImageFileNames.clear();
				imageTimes.clear();

				//reset coordinates?
				xc = 0;
				yc = 0;

				imageLoaded = false; // reset bool for next load
				g_bIsFileLoaded = false;
			}
			
			if (soundLoaded)
			{
				// display time took for sound
				float playTime = soundTimes[0];
				std::wstringstream PT;
				PT << playTime;

				// Total Time Message
				TextOut(_hWindowDC, 110, 340, L"Play Time Taken: ", 17);
				TextOut(_hWindowDC, 230, 340, PT.str().c_str(), 10);
				TextOut(_hWindowDC, 300, 340, L"milliseconds", 12);

				g_vecSoundFileNames.clear();
				soundTimes.clear();

				soundLoaded = false;
				g_bIsFileLoaded = false;
			}

		}

		EndPaint(_hwnd, &ps);

		return (0);
	}
	break;
	case WM_COMMAND:
	{
		switch (LOWORD(_wparam))
		{


		case ID_FILE_LOADIMAGE:
		{
			if (ChooseImageFilesToLoad(_hwnd))
			{
				float maxThreads = SetThreads(_hwnd); // determine max threads used from file (works better with threads::hardware_concurrency())

				bitMaps.resize(g_vecImageFileNames.size()); //Resize HBITMAP vector for amount of images loaded
				imageTimes.resize(3); // set size for stored times

				// varibales to work distribution of large no. images selected
				float imagesToLoad = g_vecImageFileNames.size();
				float loadsPerThread = 0;
				int threadsToUse = 0;
				bool isEven;
				
				// determine how many files to be loaded per thread & amount of threads to use
				if (g_vecImageFileNames.size() > maxThreads)
				{
					//determine no. of images to load on a thread
					loadsPerThread = ceil(imagesToLoad / maxThreads);

					// find optimal amount of threads to use
					threadsToUse = ceil(imagesToLoad / loadsPerThread);

					// resize thread vector to total threads to use
					myThreads.resize(threadsToUse);

					if ((int)imagesToLoad % 2 == 0)
					{
						isEven = true;
					}
					else
					{
						isEven = false;
					}
				}
				
				// Begin load timer
				auto loadTimeStart = steady_clock::now();
				if (loadsPerThread > 0) // if there are more images than total threads available to load
				{
					/* --- divide work over all available threads --- */
					int imageIndex = 0; // create image load index block
				
					for (int i = 0; i < threadsToUse; i++) // loops to start the threads
					{
						if (i == threadsToUse - 1 && !isEven) // if on last thread & an uneven amount to load
						{
							loadsPerThread = 1;
							myThreads[i] = thread(ThreadLoadImage, _hwnd, loadsPerThread, imageIndex);
						}
						//else if (i == threadsToUse - 1) // if on last thread & an uneven amount to load
						//{
						//	loadsPerThread = 2;
						//	myThreads[i] = thread(ThreadLoadImage, _hwnd, loadsPerThread, imageIndex);
						//}
						else
						{
							myThreads[i] = thread(ThreadLoadImage, _hwnd, loadsPerThread, imageIndex);
							imageIndex += loadsPerThread; // increment index for next thread start
						}
					}
				}
				else // sufficient threads are available
				{
					myThreads.resize(imagesToLoad); // resize thread vector for amount of images
					for (int i = 0; i < g_vecImageFileNames.size(); i++)
					{
						myThreads[i] = thread(loadPic, i, _hwnd);
					}
				}
				// End Load timer
				auto loadTimeEnd = steady_clock::now();
				
				for_each(myThreads.begin(), myThreads.end(), mem_fn(&thread::join)); // join threads
				myThreads.clear(); // reset for display threads

				// record time takes & store
				duration<double> totalLoadTime = loadTimeEnd - loadTimeStart;
				imageTimes[0] = totalLoadTime.count();
				
				imageLoaded = true; 

			}
			else
			{
				MessageBox(_hwnd, L"No Image File selected", L"Error Message", MB_ICONWARNING);
			}

			// Tell WM_PAINT to redraw
			RedrawWindow(_hwnd, NULL, NULL, RDW_ERASENOW | RDW_INVALIDATE | RDW_UPDATENOW);


			return (0);
		}
		break;
		case ID_FILE_LOADSOUND:
		{
			if (ChooseSoundFilesToLoad(_hwnd))
			{
				//Write code here to create multiple threads to load sound files in parallel
				float maxThreads = SetThreads(_hwnd); // determine max threads used from file (works better with threads::hardware_concurrency())

				soundTimes.resize(1); // set size for stored times

				// varibales to work distribution of large no. images selected
				float soundsToLoad = g_vecSoundFileNames.size();
				float soundsPerThread = 0;
				int threadsToUse = 0;
				bool isEven;

				// determine how many sounds to be loaded per thread & amount of threads to use
				if (g_vecSoundFileNames.size() > maxThreads)
				{
					//determine no. of images to load on a thread
					soundsPerThread = ceil(soundsToLoad / maxThreads);

					// find optimal amount of threads to use
					threadsToUse = ceil(soundsToLoad / soundsPerThread);

					// resize thread vector to total threads to use
					myThreads.resize(threadsToUse);

					if ((int)soundsToLoad % 2 == 0)
					{
						isEven = true;
					}
					else
					{
						isEven = false;
					}
				}

				// Begin load timer
				auto soundTimeStart = steady_clock::now();
				if (soundsPerThread > 0) // if there are more images than total threads available to load
				{
					/* --- divide work over all available threads --- */
					int soundIndex = 0; // create image load index block

					for (int i = 0; i < threadsToUse; i++) // loops to start the threads
					{
						if (i == threadsToUse - 1 && !isEven) // if on last thread and an uneven amount of files
						{
							soundsPerThread = 1;
							myThreads[i] = thread(PlayWAVThreaded, i, soundsPerThread);
						}
						else
						{
							myThreads[i] = thread(PlayWAVThreaded, i, soundsPerThread);
							soundIndex += soundsPerThread; // increment index for next thread start
						}
					}
				}
				else // sufficient threads are available
				{
					myThreads.resize(soundsToLoad); // resize thread vector for amount of images
					for (int i = 0; i < g_vecSoundFileNames.size(); i++)
					{
						myThreads[i] = thread(PlayWAV, i);
					}
				}
				// End play timer
				auto soundTimeEnd = steady_clock::now();

				for_each(myThreads.begin(), myThreads.end(), mem_fn(&thread::join)); // join threads
				myThreads.clear(); // reset for display threads
				g_vecSoundFileNames.clear(); // reset for new load

				// record time takes & store
				duration<double> totalSoundTime = soundTimeEnd - soundTimeStart;
				soundTimes[0] = totalSoundTime.count();

				soundLoaded = true;

			}
			else
			{
				MessageBox(_hwnd, L"No Sound File selected", L"Error Message", MB_ICONWARNING);
			}

			// Tell WM_PAINT to redraw
			RedrawWindow(_hwnd, NULL, NULL, RDW_ERASENOW | RDW_INVALIDATE | RDW_UPDATENOW);

			return (0);
		}
		break;
		case ID_EXIT:
		{
			MessageBox(_hwnd, L"Okay, bye bye.", L"You're leaving?", MB_OK);
			SendMessage(_hwnd, WM_CLOSE, 0, 0);
			return (0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_CLOSE:
	{
		PostQuitMessage(0);
	}
	break;
	default:
		break;
	}
	return (DefWindowProc(_hwnd, _uiMsg, _wparam, _lparam));
}


HWND CreateAndRegisterWindow(HINSTANCE _hInstance) // HWND == "window handle" 
{
	WNDCLASSEX winclass; // This will hold the class we create.
	HWND hwnd;           // Generic window handle.

						 // First fill in the window class structure.
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = _hInstance;
	winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground =
		static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH));
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = WINDOW_CLASS_NAME;
	winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// register the window class
	if (!RegisterClassEx(&winclass))
	{
		return (0);
	}

	HMENU _hMenu = LoadMenu(_hInstance, MAKEINTRESOURCE(IDR_MENU1));

	// Create the window
	hwnd = CreateWindowEx(NULL, // Extended style.
		WINDOW_CLASS_NAME,      // Class.
		L"MultiThreaded Loader Tool - Charles Bird",   // Title.
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		10, 10,                    // Initial x,y.
		_kuiWINDOWWIDTH, _kuiWINDOWHEIGHT,                // Initial width, height.
		NULL,                   // Handle to parent.
		_hMenu,                   // Handle to menu.
		_hInstance,             // Instance of this application.
		NULL);                  // Extra creation parameters.

	return hwnd;
}

int WINAPI WinMain(HINSTANCE _hInstance,
	HINSTANCE _hPrevInstance,
	LPSTR _lpCmdLine,
	int _nCmdShow)
{
	MSG msg;  //Generic Message

	HWND _hwnd = CreateAndRegisterWindow(_hInstance);

	if (!(_hwnd)) // if window has Not laoded properly
	{
		return (0);
	}


	// Enter main event loop
	while (true)
	{
		// Test if there is a message in queue, if so get it.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Test if this is a quit.
			if (msg.message == WM_QUIT)
			{
				MessageBox(_hwnd, L"bye", L" yep", MB_OK);
				break;
			}

			// Translate any accelerator keys.
			TranslateMessage(&msg);
			// Send the message to the window proc.
			DispatchMessage(&msg);
		}

	}

	// Return to Windows like this...
	return (static_cast<int>(msg.wParam));
}