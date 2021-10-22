
#include <Windows.h>
#include <functional> // for ref()
#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <thread>
#include "resource.h"
#include "Threadpool.h"

#define WINDOW_CLASS_NAME L"MultiThreaded Loader Tool"
const unsigned int _kuiWINDOWWIDTH = 1200;
const unsigned int _kuiWINDOWHEIGHT = 1200;
#define MAX_FILES_TO_OPEN 50
#define MAX_CHARACTERS_IN_FILENAME 25

using std::thread;
using std::vector;
using std::wstring;
using std::queue;
using std::ref;

//Global Variables
const int maxThreads = thread::hardware_concurrency(); // Create varibale for total threads
vector<wstring> g_vecImageFileNames;
vector<wstring> g_vecSoundFileNames;
HINSTANCE g_hInstance; // creating an instance of window handle
bool g_bIsFileLoaded = false; 
vector<thread> myThreads;
vector<HBITMAP> bitMaps;// = new vector<HBITMAP>[MAX_FILES_TO_OPEN];

HDC testhdcMem = NULL;
HBITMAP testhBmp = NULL;
HBITMAP testhBmpOld = NULL;
mutex gLock;

// callable within WM_COMMAND handler, or where we want the image loaded
void loadPic(int index)
{
	gLock.lock();
	bitMaps[index] = (HBITMAP)LoadImage(NULL, (LPCWSTR)g_vecImageFileNames[index].c_str(), IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE);
	gLock.unlock();
	
}

void PaintImage(HWND _hwnd, int index, int cx, int cy, HDC _hWindowDC)
{
	/*Variales for bitamp drawing*/
	HDC hdcMem = NULL;
	HBITMAP hBmpOld = NULL;

	//create the initial window to hold bitmap
	_hwnd = CreateWindow(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, 10, 10, 0, 0, _hwnd, NULL, NULL, NULL);

	// assign 
	bitMaps[index] = (HBITMAP)LoadImageW(NULL, (LPCWSTR)g_vecImageFileNames[index].c_str(),
		IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE);
	
	//let window know what it will display
	SendMessage(_hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitMaps[index]);


	if (bitMaps[index] != NULL) // only if bitmap loaded correctly
	{
		hdcMem = CreateCompatibleDC(_hWindowDC); // set up memory device context

		// get bitmaps parameters
		BITMAP bm;
		GetObject(reinterpret_cast<HGDIOBJ>(bitMaps[index]), sizeof(BITMAP), reinterpret_cast<LPVOID>(&bm));

		// select loaded bitmap
		hBmpOld = (HBITMAP)SelectObject(hdcMem, bitMaps[index]);

		// blit the dc which holds bitmap onto windows dc
		bool gBlit = BitBlt(_hWindowDC,
			cx, /*x - these get incremented on loop starting threads */
			cy, /*y - this avoids any overlap of images */
			100, /* width */
			100, /* height */
			hdcMem, 0, 0, SRCCOPY);

		// if display fails, show error message
		if (!gBlit) MessageBox(_hwnd, L"Blit Failed", L"Display Error", MB_ICONWARNING);

		// initialize and free from memory
		SelectObject(hdcMem, hBmpOld);
		DeleteDC(hdcMem);
		DeleteObject(bitMaps[index]);
	}
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
	static int cxClient, cyClient;

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
		//TODO Do all our painting here
		TextOut(_hWindowDC, 0, 0, L"Yo, Waddup", 15); // this works, and is constantly painted
		Rectangle(_hWindowDC, 100, 100, 200, 200); // this works
		RoundRect(_hWindowDC, 300, 300, 600, 600, 20, 20);

		int xc = 100; // x-coordinate for top left point of image 
		int yc = 100; // y-coordinate for top left point, where image drawn from

		if (g_bIsFileLoaded)
		{
			// loop through number of images
			for (int i = 0; i < g_vecImageFileNames.size(); i++)
			{
				myThreads.push_back(thread([=]()
				{
					//myThreads.push_back(thread(PaintImage, _hwnd, i, ref(xc), ref(yc), ref(_hWindowDC)));
					PaintImage(_hwnd, i, ref(xc), ref(yc), ref(_hWindowDC));
				}));

				xc += 200; // move next image too be painted along by 200 pixels
				if (xc >= 600)
				{
					yc += 200; // drop images to next line
					xc = 100; // reset x-coordinate
				}
			}
			
			for_each(myThreads.begin(), myThreads.end(), mem_fn(&thread::join)); // join threads
			myThreads.clear(); // reset
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
				//TODO Write code here to create multiple threads to load image files in parallel
				// 1. files are selected and added to g_vecImageFiles[].
				// 2. file paths then need to be added to HBITMAP types
				// 3. HBITMAP types then need to be loaded with the "LoadImage()" function
				// 3.5. once 'loaded', files can be drawn with 'WM_PAINT'
				// 4. start threads based on num of files selected

				for (int i = 0; i < g_vecImageFileNames.size(); i++)
				{
					// Function ready to call
				    /* myThreads.push_back(thread(loadPic, i)); */

					// Use of lambda as no callable function for image loading has been written yet.
					myThreads.push_back(thread([=]()
					{
						gLock.lock(); // lock so multiple images are not trying to be pushed to same index
						bitMaps[i] = (HBITMAP)LoadImageW(NULL, (LPCWSTR)g_vecImageFileNames[i].c_str(), IMAGE_BITMAP, 10, 10, LR_LOADFROMFILE);
						gLock.unlock();
					
					}));
				}
				
				for_each(myThreads.begin(), myThreads.end(), mem_fn(&thread::join)); // join threads. Error: "VECTOR SUBSCRIPT OUT OF RANGE"
				myThreads.clear(); // reset for display threads


				/* TESTER BELOW --------------------------------------------------------------------------------- */
				HBITMAP hBitMap = NULL;

				// Create window for bitmap display
				_hwnd = CreateWindow(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, 10, 10, 0, 0, _hwnd, NULL, NULL, NULL);
				SendMessage(_hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitMap);

				LPCWSTR fileName = (LPCWSTR)g_vecImageFileNames[0].c_str();

				// assign(load) image to bitmap handle
				hBitMap = (HBITMAP)LoadImageW(NULL, fileName, IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE); // not loading, why? bitmap handle stays null !

				//hBitMap = LoadBitmapW(g_hInstance, fileName);
				DWORD error = GetLastError(); // check error code


				// check if loaded correctly
				if (hBitMap == NULL)
				{
					if (error == 0) // The operation completed successfully, though image not loaded
					{
						MessageBox(_hwnd, L"hBitMap is Null", L"Image Load Error", MB_ICONWARNING);
					}
					else if (error == 2) // Cannot Locate
					{
						MessageBox(_hwnd, L"The system cannot find the file specified", L"Filepath Invalid", MB_ICONWARNING);
					}
					else if (error == 6) // invalid handle ??
					{
						MessageBox(_hwnd, L"Handle is Invalid", L"Window Handle Error", MB_ICONWARNING);
					}
					else
					{
						MessageBox(_hwnd, L"Load Image Failed", L"Error Unknown", MB_ICONWARNING);
					}
				}
			}
			else
			{
				MessageBox(_hwnd, L"No Image File selected", L"Error Message", MB_ICONWARNING);
			}

			return (0);
		}
		break;
		case ID_FILE_LOADSOUND:
		{
			if (ChooseSoundFilesToLoad(_hwnd))
			{
				//Write code here to create multiple threads to load sound files in parallel
			}
			else
			{
				MessageBox(_hwnd, L"No Sound File selected", L"Error Message", MB_ICONWARNING);
			}
			return (0);
		}
		break;
		case ID_EXIT:
		{
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
		_kuiWINDOWWIDTH-400, _kuiWINDOWHEIGHT-400,                // Initial width, height.
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