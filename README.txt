README Charles Bird Assessment 2

----------------------------------------------
	Multithreaded Multimedia Loader 
----------------------------------------------

NOTE:

within the solution folder there are 2 additional folders marked 'wav' & 'bmp', these contain tester images and sounds,
along with the .txt file which controls the max threads to use.
please adjust the number within the file 'threadsToUse.txt' in BOTH wav * bmp folders as the program loads from either one
depending on whether you are loading images or sounds
IF YOU WILL BE LOADING OWN IMAGES, make sure the .txt file to determine max threads is in the same folder, as this is 
how i've arranged my code.

RETROSPECT:
I was unable to successfully paint images in unison, nor play sounds in unison.
	for images, the functions CreateWindow() & SendMessage() tended to misbehave rampantly
	for sounds, I couldn't get files to play at once, they seemd to be place in an unseen queue and played
	            one after the other, of which I seemingly had no control.

OPERATIONS:
for bug-free operations load images once, close the program then load the sounds alone.
or
load sounds BEFORE you load images, this seems to behave.
be sure to read the NOTE: section above, thanks :)

Thanks for marking my work! please enjoy!



--- KNOWN ERRORS/ BUGS ---

-With max threads set to 12...
	- at 25, 26, 28, 32, 34 num of files vector subscript error
	- at 27 files, 2 images do not draw
	- at 29 files, 1 image does not draw
	- at 33 files, 2 images do not draw
	- at 35 files, 1 image does not draw

	....etc. etc. We can see the pattern.
	This will keep happening as the size of images to be loaded grows, better handling of indexes is required for
	expantion of program
	however, with max threads set higher (i.e at 24) this issue isn't encountered until nearing 45 pics to load.

- after initial load of images and displaying load times, upon loading again, new images will show but the timers do not show.

- after a load of images, when sounds are loaded, a vector subscript error is thrown.
	this is possibly due to using the myThreads vector again, but cannot locate error as proper clearing/resetting of vector
	seems to be in place.

- window doesn't seem to clear for new run of loaded images. those not painted over remain on the screen.
	unkown why these remain, when their data has been cleared from vectors, there should be no more access
 
