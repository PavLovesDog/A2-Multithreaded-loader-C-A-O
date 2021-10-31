
#include <Windows.h>
#include <functional> // for ref()
#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include "resource.h"
//#include "Threadpool.h"

#define WINDOW_CLASS_NAME L"MultiThreaded Loader Tool"
const unsigned int _kuiWINDOWWIDTH = 600;
const unsigned int _kuiWINDOWHEIGHT = 800;
#define MAX_FILES_TO_OPEN 50
#define MAX_CHARACTERS_IN_FILENAME 25

using std::thread;
using std::vector;
using std::wstring;
using std::queue;
using std::mutex;
using std::ref;

//Global Variables
const int maxThreads = thread::hardware_concurrency(); // Create varibale for total threads
vector<wstring> g_vecImageFileNames;
vector<wstring> g_vecSoundFileNames;
HINSTANCE g_hInstance; // creating an instance of window handle
bool g_bIsFileLoaded = false; 
vector<thread> myThreads;
vector<HBITMAP> bitMaps; // = new vector<HBITMAP>[MAX_FILES_TO_OPEN];

int xc = 0; // 100; // x-coordinate for top left point of image 
int yc = 0; // 100; // y-coordinate for top left point, where image drawn from

HBITMAP hBitMap;
mutex gLock; // window creation lock
mutex vLock; // coordinate value manipulation lock

/*CLASS TEST FOR THREADPOOLING*/
//class ImageTasks
//{
//public:
//
//	void loadPic(int index)
//	{
//		gLock.lock();
//		bitMaps[index] = (HBITMAP)LoadImageW(NULL, (LPCWSTR)g_vecImageFileNames[index].c_str(), IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE);
//		gLock.unlock();
//
//	}
//
//	void PaintImageNow(HWND wnd, int cx, int cy, int ImageNo)
//	{
//		gLock.lock();
//		wnd = CreateWindow(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, xc, yc, 0, 0, wnd, NULL, NULL, NULL);
//		SendMessageW(wnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitMaps[ImageNo]);
//		gLock.unlock();
//	}
//
//};

// callable within WM_COMMAND handler, or where we want the image loaded
void loadPic(int index, HWND _hwnd)
{
	gLock.lock();
	bitMaps[index] = (HBITMAP)LoadImageW(NULL, (LPCWSTR)g_vecImageFileNames[index].c_str(), IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE);
	//check for error
	DWORD error = GetLastError();
	gLock.unlock();

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

void PaintImageNow(HWND _hwnd, int ImageNo)
{
	gLock.lock();
	_hwnd = CreateWindow(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, xc, yc, 0, 0, _hwnd, NULL, NULL, NULL);
	gLock.unlock();

	SendMessageW(_hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitMaps[ImageNo]);
}

void ThreadPaintImage(HWND wnd, int ImagesPerThread, int ImageBlock)
{
	int xPos = 0;
	int yPos = 0;

	gLock.lock();

	// loop through the images needed to paint
	for (int i = 0; i < ImagesPerThread; i++)
	{
		//create window THIS MAY NEED TO NOT BE ON THREAD!? ...Then how to implement?
		wnd = CreateWindow(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, xc + xPos, yc + yPos, 0, 0, wnd, NULL, NULL, NULL); //TODO thread stalls on this??

		// send message to paint the image
		SendMessageW(wnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitMaps[i + (std::uint64_t)ImageBlock]); // cast the 4byte to 8byte to avoid overflow, recommended by intellisense 

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

// MY FAILED 1ST ATTEMPT - used online tutorials
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
	case WM_CREATE:
	{
		//_hwnd = CreateWindow(L"Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, xc, yc, 0, 0, _hwnd, NULL, NULL, NULL);
	}
	break;
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

		TextOut(_hWindowDC, 0, 0, L"Yo, Waddup", 15); // this works, and is constantly painted
		Rectangle(_hWindowDC, 100, 100, 200, 200); // this works
		RoundRect(_hWindowDC, 100, 300, 400, 400, 20, 20);

		if (g_bIsFileLoaded)
		{
			//int imagesPerThread = 0; // initialize variable for large no. images selected
			//
			//	// determine how many files to be loaded per thread
			//if (g_vecImageFileNames.size() > maxThreads)
			//{
			//	imagesPerThread = floor(g_vecImageFileNames.size() / maxThreads);
			//}
			//
			//if (imagesPerThread > 0) // if there are more images than total threads available to load
			//{
			//	//divide work over all available threads
			//	int imageIndex = 0; // create image block index
			//	for (int i = 0; i < maxThreads; i++) // loops to start the threads
			//	{
			//		ThreadPaintImage(_hwnd, imagesPerThread, imageIndex);
			//		//myThreads[i] = thread(ThreadPaintImage, _hwnd, imagesPerThread, imageIndex);
			//		imageIndex += imagesPerThread; // increment index for next thread start
			//
			//		xc += 100 * imagesPerThread; // move along by amount of pictures painted in last thread
			//		if (xc >= _kuiWINDOWWIDTH)
			//		{
			//			yc += 100; // drop images to next line
			//			xc = 0; // reset x-coordinate
			//		}
			//	}
			//}
			//else // sufficient threads are available
			{
				// run work on number of threads dictated by vector size
				for (int i = 0; i < bitMaps.size(); i++)
				{
					//Controller_Multi(_hwnd, i);

					PaintImageNow(_hwnd, i); /* THIS WORKS */

					//alter variables for image paint on next loop
					xc += 100; // move next image too be painted along by 100 pixels
					if (xc >= _kuiWINDOWWIDTH)
					{
						yc += 100; // drop images to next line
						xc = 0; // reset x-coordinate
					}
				}

				//clear bitmap & string vector for if user chooses to load more images
				bitMaps.clear();
				g_vecImageFileNames.clear();
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
				//Resize vector for images
				bitMaps.resize(g_vecImageFileNames.size());

				for (int i = 0; i < g_vecImageFileNames.size(); i++)
				{
					// Function ready to call
				    myThreads.push_back(thread(loadPic, i, _hwnd)); //TODO SET LIMIT TO HARDWARE CONCURRENCY, SPLIT MULTIPLE LOADS INTO THREADS

					//check for error
					DWORD error = GetLastError();

					// check if loaded correctly
					if (bitMaps[i] == NULL)
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
				
				for_each(myThreads.begin(), myThreads.end(), mem_fn(&thread::join)); // join threads
				myThreads.clear(); // reset for display threads

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