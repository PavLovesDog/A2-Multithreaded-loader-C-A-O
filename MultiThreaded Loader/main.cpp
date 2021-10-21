
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

//Global Variables
const int maxThreads = thread::hardware_concurrency(); // Create varibale for total threads
vector<wstring> g_vecImageFileNames;// = new vector<wstring>[MAX_FILES_TO_OPEN];
vector<wstring> g_vecSoundFileNames;// = new vector<wstring>[MAX_FILES_TO_OPEN];
HINSTANCE g_hInstance; // creating an instance of window handle
bool g_bIsFileLoaded = false; // USE THIS WITH LOADING FILES?
//bool g_isImageFile = false; // bools for loading file types in queue THESE WOULD BE FOR THREADPOOL HEADER USE
//bool g_isSoundFile = false;	// bools for loading file types in queue THESE WOULD BE FOR THREADPOOL HEADER USE
vector<thread> myThreads;// = new vector<thread>[maxThreads]; // set up vector for threads with capacity on heap.

HDC testhdcMem = NULL;
HBITMAP testhBmp = NULL;
HBITMAP testhBmpOld = NULL;


// callable within WM_COMMAND handler, or where we want the image loaded
void loadPic()
{
	if (testhdcMem) return; /*picture already loaded, don't load again*/
	
	wstring currentBmp = g_vecImageFileNames[0];
	
	testhBmp = (HBITMAP)LoadImage(NULL, (LPCWSTR)g_vecImageFileNames[0].c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); // must need to change to load from a vector
	
	if (!testhBmp) return; /*load failed, can't find file or invalid*/
	
	testhdcMem = CreateCompatibleDC(NULL);
	testhBmpOld = (HBITMAP)SelectObject(testhdcMem, testhBmp);
}

class Tasks {
public:
	void push_image(wchar_t* _wsFileToOpen)
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

bool ChooseImageFilesToLoad(HWND _hwnd) // passing in the windows handle
{
	OPENFILENAME ofn;
	//TODO setting the MEMORY SIZE of variables
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1]; // wchar_t is a wide char... 
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
	ofn.lpstrFile[0] = '\0'; // set first file buffer to NULL
	ofn.lpstrFilter = L"Bitmap Images(.bmp)\0*.bmp\0"; //Filter for bitmap images, defines the file type to load
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT; // set flags of the file function

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//First way: just work with wide char arrays
		wcsncpy_s(_wsPathName, wsFileNames, ofn.nFileOffset); //TODO this is copying the file name? or 'extracting' file name?
		int i = ofn.nFileOffset;
		int j = 0;

		//TODO Would this while loop start on threads?? 
		while (true)
		{
			if (*(wsFileNames + i) == '\0')
			{
				_wstempFile[j] = *(wsFileNames + i);
				wcscpy_s(_wsFileToOpen, _wsPathName);
				wcscat_s(_wsFileToOpen, L"\\");
				wcscat_s(_wsFileToOpen, _wstempFile);
				
				//BELOW CODE WORKS
				// Use Lambda Expression to construct callable code for thread
				myThreads.emplace_back(thread([=]()
					{
						g_vecImageFileNames.push_back(_wsFileToOpen); //TODO OG. this is where the file is pushed to the vector storing file names
				}));
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
	volatile HDC _hWindowDC;
	static BITMAP bm;
	static int cxClient, cyClient;

	/*Variales for bitamp drawing*/
	HDC hdcMem = NULL;
	HBITMAP hBmp = NULL;
	HBITMAP hBmpOld = NULL;
	//wchar_t currentFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	//wstring currentBmp;
	//LPCWSTR fileName;

	//RECT rect;
	switch (_uiMsg)
	{
	case WM_CREATE:
	{
		//TODO FAIL, MAY NOT NEED WM_CREATE?
		//// load the bitmap from file stored in vector
		//hBmp = (HBITMAP)LoadImage(NULL, L"C:\\Users\\charl\\Pictures\\\\doggie-bmp.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		//
		//if (hBmp)
		//{
		//	GetObject(hBmp, sizeof(BITMAP), (PSTR)&bm); // get info of loaded bitmap
		//}
		//
		//return 0;
	}
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

		//TODO FAIL
		//if (hBmp) // only if bitmap loaded correctly
		//{
		//	//memory DC and select bitmap
		//	hdcMem = CreateCompatibleDC(_hWindowDC);
		//	SelectObject(hdcMem, hBmp);
		//
		//	BitBlt(_hWindowDC,
		//		max(0, (cxClient - bm.bmWidth) / 2),
		//		max(0, (cyClient - bm.bmHeight) / 2),
		//		min(cxClient, bm.bmWidth),
		//		min(cyClient, bm.bmHeight),
		//		hdcMem, 0, 0, SRCCOPY);
		//
		//	DeleteDC(hdcMem);
		//}

		//TODO FAIL
		//StretchBlt(_hWindowDC,  // destination DC
		//	120,				// x upper left
		//	10,					// y upper left
		//	640,				// destination width
		//	480,				// destination height
		//	hdcMem,				// the HDC created globally
		//	0,					// x & y upper left
		//	0,					//
		//	640,				// source bitmap width
		//	480,				// source bitmap height
		//	SRCCOPY);			// raster operation

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
				// 1. create vector
				// 2. file paths then need to be added to HBITMAP types
				// 3. HBITMAP types then need to be loaded with the "LoadImage()" function
				// 3.5. once 'loaded', files can be drawn with 'WM_PAINT'
				// 4. start threads based on num of files selected

				//HBITMAP hBitMap = NULL; // create the bitmap handle

				

				// convert wstring to LPCWSTR code source: https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
				wstring currentBmp = g_vecImageFileNames[0];
				LPCWSTR fileName = currentBmp.c_str();

				//LPCWSTR fileName = L"C:\\Users\\charl\\Documents\\\\doggie.bmp";   /*TESTER, DOESN'T WORK*/
				//Lock the function as it loads...

				// assign (load) image to bitmap handle
				HBITMAP hBitMap = (HBITMAP)LoadImageW(g_hInstance, (LPCWSTR)g_vecImageFileNames[0].c_str(),
					IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE); // this is not loading, why? bitmap handle stays null !

				// check if loaded correctly
				if (hBitMap == NULL)
				{
					MessageBox(_hwnd, L"Load Image Failed", L"Error", MB_ICONWARNING);
				}

				
				// run through vector and load on individual threads 
				//start thread loading one image from vector
	
					//myThreads.push_back(thread(loadImageFunction(some params)));
					//USE LAMBDA
				//
					
					// where am I loading images too??? and how??
					// loading them to the window?

				//for_each(myThreads.begin(), myThreads.end(), mem_fn(&thread::join)); // join threads

				loadPic();
				InvalidateRect(_hwnd, NULL, NULL);

				if (testhBmp == NULL)
				{
					MessageBox(_hwnd, L"Test Failed", L"Error", MB_ICONWARNING);
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