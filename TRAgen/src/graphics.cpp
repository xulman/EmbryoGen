/*
Basic diffusion and FRAP simulation
 Jonathan Ward and Francois Nedelec, Copyright EMBL Nov. 2007-2009

 
To compile on the mac:
 g++ main.cc -framework GLUT -framework OpenGL -o frap

To compile on Windows using Cygwin:
 g++ main.cc -lopengl32 -lglut32 -o frap
 
To compile on Linux:
 g++ main.cc -lglut -lopengl -o frap


Considerably further modified by Vladimir Ulman, 2015.
To compile add also -lglew switch.
*/

#include <GL/glew.h>
#include <GL/glut.h>
#include <list>
#include <vector>
#include <iostream>
#include <math.h>
#include <stdio.h>	 //for sprintf()
#include <string.h>

#include "params.h"
#include "agents.h"
#include "simulation.h"
#include "graphics.h"	//for closeGL()

#ifdef RENDER_TO_IMAGES
  #include <i3d/image3d.h>
  #include <i3d/filters.h> //for Gaussians()
  #include <i3d/morphology.h> //for Erosion in PhC
  #include <i3d/toolbox.h> //for SwapVert()
#endif
#ifdef READ_TEXTURES
  #include <i3d/image3d.h>
  #include <map>
  #include <fstream>
#endif

//link to the global params
extern ParamsClass params;

//link to the list of all agents
extern std::list<Cell*> agents;

///whether the forces should be displayed, one bit for one force
int showForces=0;
float forceStretchFactor=1.f;
float velocityStretchFactor=1.f;

///whether the current and/or former cell positions should be displayed
int showCells=1; //&1 = show current, &2 = show former,
                 //&4 = show circle, &8 = show current points

///whether the simulation is paused (=1) or not (=0)
int pauseSimulation=1;
///how big chunks of steps should be taken between drawing
int stepGranularity=10;

///whether to show timestamp in the bottom-right corner
int showTime=1;
int showCellCount=1;

///OpenGL window size -> to recalculate from windows coords to scene coords
int windowSizeX=600, windowSizeY=600;

///menu handlers:
int startMenu, controlsMenu, displayMenu, aboutMenu, mainMenu;

#ifdef DEBUG_CALCDISTANCE2
 //link to debug vectors from agents.cpp
 extern std::vector<Vector3d<float> > pointsToShow;
 extern std::vector<Vector3d<float> > pairsToShow;
#endif

#ifdef RENDER_TO_IMAGES
 //counter to get continous file name indices
 int frameCnt=0;
#endif


//------------------------
bool displayTextureOutputInsteadOfOutlines=false;

// Da Framebuffer (to render for exporting to images)
GLuint frameBuffer;
GLuint rb[2];

// texture "pointers"
GLuint tex[5000];
int texTotal=-1;

GLint posAttrib, texAttrib, colAttrib;

//inidices for the different rendering programs
GLuint defaultProgram, shaderProgram;
//------------------------



///manage \e count steps of the simulation and then print and draw
void step(int count)
{
	while ((params.currTime < params.stopTime) && (count > 0))
	{
#ifdef DEBUG_CALCDISTANCE2
		pointsToShow.clear();
		pairsToShow.clear();
#endif

		moveAgents(params.incrTime);
		params.currTime += params.incrTime;
		--count;
	}
	
	if (params.currTime >= params.stopTime)
	{
		REPORT("END OF THE SIMULATION HAS BEEN REACHED");
		pauseSimulation=1;
	}

	REPORT("now at time: " << params.currTime
		<< " min; there are currently " << agents.size()
		<< " cells in the system");

	std::list<Cell*>::const_iterator c=agents.begin();
	for (; c != agents.end(); c++)
		(*c)->ReportState();

	//ask GLUT to call the display function
	glutPostRedisplay();
}


#ifdef DEBUG_CALCDISTANCE2
/**
 * draws the processed points and pairs of the selected cells,
 *
 * basically, it reads the pointsToShow and pairsToShow structures
 * that Cell::CalcDistance2() is possibly filling
 */
void displayDistanceDebug(void)
{
	std::vector<Vector3d<float> >::const_iterator p;

	glColor3f(1.f,1.f,1.f);
	glBegin(GL_LINES);
	p=pairsToShow.begin();
	while (p != pairsToShow.end())
	{
		glVertex3f(p->x,p->y,p->z);
		++p;
	}
	glEnd();

	glPointSize(3.f);
	glBegin(GL_POINTS);
	p=pointsToShow.begin();
	while (p != pointsToShow.end())
	{
		glVertex3f(p->x,p->y,p->z);
		++p;
	}
	glEnd();
}
#endif


///draws the frame around the playground
void displayEdges(void)
{
    glColor3f(params.sceneBorderColour.r,
	 			  params.sceneBorderColour.g,
	 			  params.sceneBorderColour.b);
    glLineWidth(1);
    glBegin(GL_LINE_LOOP);
    glVertex2f( params.sceneOffset.x , params.sceneOffset.y );
    glVertex2f( params.sceneOffset.x + params.sceneSize.x , params.sceneOffset.y );
    glVertex2f( params.sceneOffset.x + params.sceneSize.x , params.sceneOffset.y + params.sceneSize.y );
    glVertex2f( params.sceneOffset.x , params.sceneOffset.y + params.sceneSize.y );
    glEnd();
}

///draws the current time into a corner
void displayTime(void)
{
	//"disassemble" the time...
	int cT=(int)round(params.currTime*10.f);
	int hours  =cT/600;
	int minutes=(cT - 600*hours)/10;
	int seconds=cT%10;

	//... and convert it to string ...
	char msg[30];
	const int textLength=16;
	sprintf(msg,"%3d h %2d.%d min    ",hours,minutes,seconds);

	//... and place it and render it
		glRasterPos2f(params.sceneOffset.x + 
						  params.sceneSize.x - 
						  textLength*3, 
						  params.sceneOffset.y - 8.0f);
	for (int i=0; i < textLength; ++i)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,msg[i]);
}

///draws the current number of cells in the scene into a corner
void displayCellCount(void)
{
	//convert the number to string ...
	char msg[20];
	const int textLength=13;
	sprintf(msg,"cells: %lu       ",agents.size());

		glRasterPos2f(params.sceneOffset.x, 
						  params.sceneOffset.y - 8.0f);

	//... and place it and render it
	for (int i=0; i < textLength; ++i)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,msg[i]);
}

///draws text telling user to click for menu
void displayHintText(void)
{
	char msg[]="Right-click for help";
	const int textLength=20;

		glRasterPos2f(params.sceneOffset.x +50.f, 
						  params.sceneOffset.y - 8.0f);

	for (int i=0; i < textLength; ++i)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,msg[i]);
}

#ifdef RENDER_TO_IMAGES
//returns true if OpenGL is rendering some new simulated stuff
bool shouldWriteScreenToImageFile(void)
{
	static float lastWriteTime=0.f;
	
	bool flag=(params.currTime > lastWriteTime);
	lastWriteTime=params.currTime;

	return (flag);
}

//the image to hold the copy of the OpenGL content and to be saved to a file
i3d::Image3d<i3d::GRAY16> imgFrame; //main output (OpenGL renders directly here)

i3d::Image3d<i3d::GRAY16> imgMask; //main output
i3d::Image3d<float> imgFloatMask;  //aux output (OpenGL renders here)

i3d::Image3d<i3d::RGB> imgOutline; //main output (OpenGL renders directly here)

//allocates memory to the current size of the OpenGL window
void initiateImageFile(void)
{
	imgFrame.MakeRoom(params.imgSizeX,params.imgSizeY,1);
	imgFrame.SetResolution(i3d::Resolution(params.imgResX,params.imgResY,1.f));
	imgFrame.SetOffset(i3d::Offset(0.f,0.f,0.f));

	imgMask.MakeRoom(params.imgSizeX,params.imgSizeY,1);
	imgMask.SetResolution(i3d::Resolution(params.imgResX,params.imgResY,1.f));
	imgMask.SetOffset(i3d::Offset(0.f,0.f,0.f));

	imgFloatMask.MakeRoom(params.imgSizeX,params.imgSizeY,1);
	imgFloatMask.SetResolution(i3d::Resolution(params.imgResX,params.imgResY,1.f));
	imgFloatMask.SetOffset(i3d::Offset(0.f,0.f,0.f));

	imgOutline.MakeRoom(params.imgSizeX,params.imgSizeY,1);
	imgOutline.SetResolution(i3d::Resolution(params.imgResX,params.imgResY,1.f));
	imgOutline.SetOffset(i3d::Offset(0.f,0.f,0.f));
}

//write content of the image to a (next) file
void writeScreenToImageFile(void)
{


	char fn[1024]; //hope 1 kB buffer for output file names is enough

	//set filename for the outline image
	sprintf(fn,params.imgOutlineFilename.c_str(),frameCnt);
	//swap upside-down (as the top-left corner differs
	//in the OpenGL rendering and the way we write to images)
	i3d::SwapVert(imgOutline.GetSizeX(),imgOutline.GetSizeY(),
	                           imgOutline.GetFirstVoxelAddr());
	imgOutline.SaveImage(fn);


	//set filename for the mask image
	sprintf(fn,params.imgMaskFilename.c_str(),frameCnt);
	//convert from float to GRAY16 "as is"
	i3d::FloatToGrayNoWeight(imgFloatMask,imgMask);
	//and swap
	i3d::SwapVert(imgMask.GetSizeX(),imgMask.GetSizeY(),
	                        imgMask.GetFirstVoxelAddr());
	//and save
	imgMask.SaveImage(fn);


	//set filename for the texture image (=phantom)
	sprintf(fn,params.imgPhantomFilename.c_str(),frameCnt);
	//and swap
	i3d::SwapVert(imgFrame.GetSizeX(),imgFrame.GetSizeY(),
	                         imgFrame.GetFirstVoxelAddr());
	//and save
	imgFrame.SaveImage(fn);


#ifdef RENDER_FLUORESCENCE
	//some phase II  postprocessing (simulation of the optics):
	i3d::Image3d<float> tmp;
	tmp.CopyMetaData(imgFrame);

	//running pointers...
	float* pF=tmp.GetFirstVoxelAddr();
	i3d::GRAY16* pV=imgFrame.GetFirstVoxelAddr();
	i3d::GRAY16* const LastpV=imgFrame.GetFirstVoxelAddr() + imgFrame.GetImageSize();
	const i3d::GRAY16* pM=imgMask.GetFirstVoxelAddr();

	for (; pV != LastpV; ++pF, ++pV, ++pM) {
		*pF=float(*pV)/256.f; //copy from integer to float image
		if (*pM) 
		{
			//some enhancing of the "signal" of the digital phantom
			*pF += 0.2f;
			*pF *= 80.0f;
		}
	}

	//tmp.SaveImage("test.tif");
	i3d::GaussIIR(tmp, 1.6f, 1.6f, 0.f);	//2D filtering - aka PSF blur


	//some phase III postprocessing (simulation of the image formation in the CCD camera):
	int signal, darkCurrNoise, readOutNoise;

	//running pointers...
	pF=tmp.GetFirstVoxelAddr();
	pV=imgFrame.GetFirstVoxelAddr();
	i3d::GRAY16* const LastpVf=imgFrame.GetFirstVoxelAddr() + imgFrame.GetImageSize();

	for (; pV != LastpVf; ++pF, ++pV)
	{
		float noiseMean = sqrtf(*pF), // from statistics: shot noise = sqrt(signal) 
				noiseVar = noiseMean; // for Poisson distribution E(X) = D(X)
		signal = int(ceilf(*pF + float(GetRandomPoisson(noiseMean)) - noiseVar));
		darkCurrNoise = int( GetRandomPoisson(0.06f) ); //tunning is specific of some old CCD hardware
		readOutNoise = int( GetRandomGauss(700.f,90.f) );

		*pV = (i3d::GRAY16)(signal + darkCurrNoise + readOutNoise);
	}

	//set filename for the fluorescence image 
	sprintf(fn,params.imgFluoFilename.c_str(),frameCnt);
	//and save
	imgFrame.SaveImage(fn);
#endif //after RENDER_FLUORESCENCE

#ifdef RENDER_PHASECONTRAST
	/*
	 * 16bit version
	 *

	i3d::GRAY16* inP=imgMask.GetFirstVoxelAddr();
	i3d::GRAY16* const inP_stop=inP + imgMask.GetImageSize();

	//turn the mask into a "rough" PhC image
	while (inP != inP_stop)
	{
		//in mask?
		if (*inP > 0) *inP=255;
		//in background
		else *inP=132;

		++inP;
	}

	//do erosion on mask by 2px
	i3d::Image3d<i3d::GRAY16> erodedImg;
	i3d::ErosionO(imgMask,erodedImg,2);

					  inP=imgMask.GetFirstVoxelAddr();
	i3d::GRAY16* eroP=erodedImg.GetFirstVoxelAddr();

	//darken the eroded stripe
	while (inP != inP_stop)
	{
		//in eroded/"vanished" pixels?
		if ((*inP == 255) && (*eroP < 255)) *inP=109;

		++inP; ++eroP;
	}

	//Gauss, and we're done
	i3d::GrayGaussIIR(imgMask,1.f);

	//set filename for the phase-contrast image 
	sprintf(fn,params.imgPhCFilename.c_str(),frameCnt);
	//and save
	imgMask.SaveImage(fn);

	 *
	 * 16bit version
	 */

	/*
	 * 8bit version
	 */
	i3d::Image3d<i3d::GRAY8> imgMask8b;
	imgMask8b.CopyMetaData(imgMask); //also allocates memory

	i3d::GRAY16* inP16b=imgMask.GetFirstVoxelAddr();
	i3d::GRAY8* inP=imgMask8b.GetFirstVoxelAddr();
	i3d::GRAY8* const inP_stop=inP + imgMask8b.GetImageSize();

	//turn the mask into a "rough" PhC image
	while (inP != inP_stop)
	{
		//in mask?
		if (*inP16b > 0) *inP=255;
		//in background
		else *inP=132;

		++inP; ++inP16b;
	}

	//do erosion on mask by 2px
	i3d::Image3d<i3d::GRAY8> erodedImg;
	i3d::ErosionO(imgMask8b,erodedImg,2);

					 inP=imgMask8b.GetFirstVoxelAddr();
	i3d::GRAY8* eroP=erodedImg.GetFirstVoxelAddr();

	//darken the eroded stripe
	while (inP != inP_stop)
	{
		//in eroded/"vanished" pixels?
		if ((*inP == 255) && (*eroP < 255)) *inP=109;

		++inP; ++eroP;
	}

	//Gauss, and we're done
	i3d::GrayGaussIIR(imgMask8b,1.f);

	//set filename for the phase-contrast image 
	sprintf(fn,params.imgPhCFilename.c_str(),frameCnt);
	//and save
	imgMask8b.SaveImage(fn);
#endif //after RENDER_PHASECONTRAST

	//updates counter
	++frameCnt;
}
#endif


void display(void)
{
	//first, render cells with texture and their masks OFF the screen
	glBindFramebuffer(GL_FRAMEBUFFER,frameBuffer);
	glUseProgram(shaderProgram);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw the elements/cells
	std::list<Cell*>::const_iterator iter=agents.begin();
	while (iter != agents.end())
	{
		//should we regenerate the whole OpenGL receipt to draw the cell?
		if ( (*iter)->VAO_isUpdate == false ) (*iter)->VAO_RefreshAll();
		//or is it enough only to adjust its position within the scene?
		//(this branch should be visited the most frequently)
		else (*iter)->VAO_UpdateOnlyBP();

		//consistency check, may be removed someday TODO
		//show the default always present texture...
		GLuint myTexHandler=0;
		//...unless the required texture is within the scope of preloaded textures
		if ((*iter)->VAO_texID < texTotal) myTexHandler=(*iter)->VAO_texID;

		glBindVertexArray( (*iter)->VAO_vao );
		glBindTexture(GL_TEXTURE_2D, tex[myTexHandler]);
		glDrawElements(GL_TRIANGLE_FAN, (*iter)->VAO_elLength, GL_UNSIGNED_INT, 0);

		iter++;
	}

	glBindVertexArray(0);


#ifdef RENDER_TO_IMAGES
	//if we should write, then copy out the first part into the images
	const bool writeFlag=shouldWriteScreenToImageFile();
	if (writeFlag)
	{
		//copies OpenGL mem(s) to the image mem
		glReadBuffer(GL_COLOR_ATTACHMENT0); //texture
		//glReadPixels(0,0,params.imgSizeX,params.imgSizeY,GL_RGB,GL_BYTE,imgRGBFrame.GetFirstVoxelAddr());
		glReadPixels(0,0,
						 GLsizei(params.imgSizeX),
						 GLsizei(params.imgSizeY),
						 GL_RED,GL_UNSIGNED_SHORT,imgFrame.GetFirstVoxelAddr());

		glReadBuffer(GL_COLOR_ATTACHMENT1); //mask ID
		glReadPixels(0,0,
						 GLsizei(params.imgSizeX),
						 GLsizei(params.imgSizeY),
						 GL_RED,GL_FLOAT,imgFloatMask.GetFirstVoxelAddr());
	}
#endif


	//now, render on the screen
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	glClear(GL_COLOR_BUFFER_BIT);
if (displayTextureOutputInsteadOfOutlines)
{
	//render texture ON the screen
	//
	//keep the same rendering setting (only the target Framebuffer is changed)
	// Draw the elements/cells
	iter=agents.begin();
	while (iter != agents.end())
	{
		//consistency check, may be removed someday TODO
		//show the default always present texture...
		GLuint myTexHandler=0;
		//...unless the required texture is within the scope of preloaded textures
		if ((*iter)->VAO_texID < texTotal) myTexHandler=(*iter)->VAO_texID;

		glBindVertexArray( (*iter)->VAO_vao );
		glBindTexture(GL_TEXTURE_2D, tex[myTexHandler]);
		glDrawElements(GL_TRIANGLE_FAN, (*iter)->VAO_elLength, GL_UNSIGNED_INT, 0);

		iter++;
	}

   glBindVertexArray(0);
}
else
{
	//render outlines ON the screen
	glUseProgram(defaultProgram);

    displayEdges();
	 if (showTime) displayTime();
	 if (showCellCount) displayCellCount();
	 if (params.currTime < 0.01f) displayHintText();

	//iterate over all cells and display them
	iter=agents.begin();
	while (iter != agents.end())
	{
		(*iter)->DrawCell(showCells);
		(*iter)->DrawForces(showForces,forceStretchFactor,velocityStretchFactor);
		iter++;
	}

#ifdef DEBUG_CALCDISTANCE2
	 displayDistanceDebug();
#endif

#ifdef RENDER_TO_IMAGES
	//if we should write, then copy out the second part into the images
	if (writeFlag)
		glReadPixels(0,0,
						 GLsizei(params.imgSizeX),
						 GLsizei(params.imgSizeY),
						 GL_RGB,GL_UNSIGNED_BYTE,imgOutline.GetFirstVoxelAddr());
#endif
}

#ifdef RENDER_TO_IMAGES
	//if we should write, what ever we have managed to save into file, now store on the harddrive
	if (writeFlag)
		//writes image mem to a file
		writeScreenToImageFile();
#endif

    glFlush();
    glutSwapBuffers();
}


// called automatically after a certain amount of time
void timer(int)
{
	 //do simulation and then draw
    step(stepGranularity);

    //ask GLUT to call 'timer' again in 'delay' milli-seconds:
	 //in the meantime, you can inspect the drawing :-)
    if (!pauseSimulation) glutTimerFunc(500, timer, 0);
}

/*
void motion(int mx, int my)
{
    //calculate position in the simulated coordinates:
    float x = xVis * ( mx * 2.0 / glutGet(GLUT_WINDOW_WIDTH) - 1 );
    float y = yVis * ( 1 - my * 2.0 / glutGet(GLUT_WINDOW_HEIGHT) );
    //bleach(x, y);
}

// called when the mouse button is pressed down or up
void click(int button, int state, int mx, int my) 
{
    if ( button == GLUT_LEFT_BUTTON ) 
        motion(mx, my);
}
*/

void GetSceneViewSize(const int w,const int h,
							float& xVisF, float& xVisT,
							float& yVisF, float& yVisT)
{
	 //what is the desired size of the scene
	 const float xBound=params.sceneSize.x
	 				+ 2.f*params.sceneOuterBorder.x;
	 const float yBound=params.sceneSize.y
	 				+ 2.f*params.sceneOuterBorder.y;

	 xVisF = params.sceneOffset.x - params.sceneOuterBorder.x;
	 yVisF = params.sceneOffset.y - params.sceneOuterBorder.y;
	 xVisT = xBound + xVisF;
	 yVisT = yBound + yVisF;

    if ( (float)w * yBound < (float)h * xBound )		//equals to  w/h < x/y
	 {
	 	//window is closer to the rectangle than the scene shape
		  yVisF *= ((float)h / float(w)) * (xBound/yBound);
        yVisT *= ((float)h / float(w)) * (xBound/yBound);
	 }
	 else
	 {
		  xVisF *= ((float)w / float(h)) * (yBound/xBound);
        xVisT *= ((float)w / float(h)) * (yBound/xBound);

	 }
}


void printKeysHelp(void)
{
	std::cout << "\nKeys:\n";
	std::cout << "ESC or 'q': quit\n";
	std::cout << "SPACEBAR and 'z' run/pause simulation ('z'=bigger chunks); 'n' pause and step simulation\n";
	std::cout << "'r' reset screen size; 'h' prints this help; 'd' prints what features are displayed; 'T' toggle time stamp\n";
	std::cout << "'s' select/deselect cell under mouse cursor; 'S' deselect all selected cells; 't' toggle outline/texture mode\n";
	std::cout << "'l' lists current boundary points of selected cells; 'i'/'I' inspects state/tells ID of cell under mouse cursor\n";
	std::cout << "'f' show/hide previous cell positions; 'F' show/hide current cell positions; 'C' toggle number of cells\n";
	std::cout << "'c' show/hide approximating circles; 'p' show/hide current boundary points\n";
	std::cout << "'1'--'9' show/hide particular force; '-','+' decrease,increase drawn force vectors\n";
	std::cout << "'[',']' decrease,increase drawn velocity vectors; '0' toggle display between forces of selected or all cells\n";
	std::cout << "LEFT,RIGHT KEY turn cell under cursor 30deg left,right; 'k' kills a cell\n\n";
}


void printDispayedFeauters(void)
{
	if (pauseSimulation) std::cout << "\nsimulation is now paused\n";
	else std::cout << "\nsimulation is now in progress\n";
	std::cout << "simulation time progress " << params.currTime/params.stopTime*100.f
		<< "%, delta time is " << params.incrTime << " minutes\n";

	std::cout << "Currently displayed features:\n";
	if (showCells & 2) std::cout << "previous cell positions (in darker contours)\n";
	if (showCells & 1) std::cout << "current  cell positions (in lighter contours)\n";
	if ((showCells & 5) == 5) std::cout << "approximating circles around current cell positions\n";
	if ((showCells & 9) == 9) std::cout << "points at the current cell positions\n";

	if (showForces == 0) return;
	std::cout << "some forces with stretch factors: forces " << forceStretchFactor
	          << "x, velocity " << velocityStretchFactor << "x, colour legend:\n";
	if (showForces & forceNames::repulsive) std::cout << "'1': repulsive = gray      ";
	if (showForces & forceNames::attractive) std::cout << "'2': attraction = red         ";
	if (showForces & forceNames::body) std::cout << "'3': body Ph1 = white";
	std::cout << "\n";

	if (showForces & forceNames::sliding) std::cout << "'4': sliding Ph2 = green   ";
	if (showForces & forceNames::friction) std::cout << "'5': friction = blue          ";
	if (showForces & forceNames::boundary) std::cout << "'6': boundary = dark green";
	std::cout << "\n";

	if (showForces & forceNames::desired) std::cout << "'7': desired = magenta     ";
	if (showForces & forceNames::final) std::cout << "'8': resulting force = cyan   ";
	if (showForces & forceNames::velocity) std::cout << "'9': current velocity = dark magenta";

	if (showForces & (int(1) << 25)) 
		std::cout << "\nunknown = dark cyan; only forces of selected cells\n\n";
	else
		std::cout << "\nunknown = dark cyan; forces of all cells\n\n";
}


// called when a key is pressed
void keyboard(unsigned char key, int mx, int my)
{
	//required for calculating the mouse position within the scene coords
	float xVisF, yVisF;
	float xVisT, yVisT;
	float sx,sy;

	//required for sweeping the cell list
	std::list<Cell*>::iterator c;
	std::list<Cell*>::const_iterator cc;

    switch (key) {
        case 27:   //this corresponds to 'escape'
        case 'q':  //quit
				closeAgents();
				closeGL();
            exit(EXIT_SUCCESS);
            break;

		  //control execution of the simulation
        case ' ':  //pause/unpause, refresh every 10 cycles
				if (params.currTime < params.stopTime) pauseSimulation^=1;
				if (!pauseSimulation)
				{
					stepGranularity=10; //= 1 min
					timer(0);
				}
            break;
        case 'z':  //pause/unpause, refresh every 600 cycles
				if (params.currTime < params.stopTime) pauseSimulation^=1;
				if (!pauseSimulation)
				{
					stepGranularity=100; //= 10 min
					timer(0);
				}
            break;
        case 'n':  //pause, advance by one frame
				pauseSimulation=1;
				step(1);
            break;

		  //etc stuff...
        case 'r':  //set scale 1:1
				glutReshapeWindow(int(params.imgSizeX),int(params.imgSizeY));
				glutPostRedisplay();
            break;
			case 'h': //print help
				printKeysHelp();
				break;
			case 'd': //print what is displayed
				printDispayedFeauters();
				break;
			case 'l': //list BPs
				for (cc=agents.begin(); cc != agents.end(); cc++)
					(*cc)->ListBPs();
				break;
			case 'i': //inspect a cell
			case 'I': //tell its ID
				GetSceneViewSize(windowSizeX,windowSizeY, xVisF,xVisT,yVisF,yVisT);
				sx=(float)mx              /(float)windowSizeX * (xVisT-xVisF) + xVisF;
				sy=(float)(windowSizeY-my)/(float)windowSizeY * (yVisT-yVisF) + yVisF;

				for (c=agents.begin(); c != agents.end(); c++)
					if ((*c)->IsPointInCell(Vector3d<float>(sx,sy,0)))
					{
						if (key == 'i')
						{
							bool wasSelected=(*c)->isSelected;
							(*c)->isSelected=true;
							(*c)->ReportState();
							(*c)->isSelected=wasSelected;
						}
						else
						{
							std::cout << "---------- cell ID=" << (*c)->ID << " ----------\n";
						}
					}
				break;
			case 13: //Enter key -- just empty lines
				std::cout << std::endl;
				break;

			//what to display:
			case 'F': //show current position
				showCells^=1;
				glutPostRedisplay();
				break;
			case 'f': //show previous position
				showCells^=2;
				glutPostRedisplay();
				break;
			case 'c': //show aproximating circles
				showCells^=4;
				glutPostRedisplay();
				break;
			case 'p': //show current position
				showCells^=8;
				glutPostRedisplay();
				break;
			case 's': //select a cell
				GetSceneViewSize(windowSizeX,windowSizeY, xVisF,xVisT,yVisF,yVisT);
				sx=(float)mx              /(float)windowSizeX * (xVisT-xVisF) + xVisF;
				sy=(float)(windowSizeY-my)/(float)windowSizeY * (yVisT-yVisF) + yVisF;

				for (c=agents.begin(); c != agents.end(); c++)
					if ((*c)->IsPointInCell(Vector3d<float>(sx,sy,0)))
						(*c)->isSelected^=true;

				glutPostRedisplay();
				break;
			case 'S': //deselect all cells
				for (c=agents.begin(); c != agents.end(); c++)
					(*c)->isSelected=false;
				glutPostRedisplay();
				break;
			case 'C': //toggle number of cells
				showCellCount^=1;
				glutPostRedisplay();
				break;
			case 'T': //toggle time stamp
				showTime^=1;
				glutPostRedisplay();
				break;
			case 't': //toggle outline/texture mode
				displayTextureOutputInsteadOfOutlines^=true;
				glutPostRedisplay();
				break;

			case '1':
				showForces ^= forceNames::repulsive;
				glutPostRedisplay();
            break;
			case '2':
				showForces ^= forceNames::attractive;
				glutPostRedisplay();
            break;
			case '3':
				showForces ^= forceNames::body;
				glutPostRedisplay();
            break;
			case '4':
				showForces ^= forceNames::sliding;
				glutPostRedisplay();
            break;
			case '5':
				showForces ^= forceNames::friction;
				glutPostRedisplay();
            break;
			case '6':
				showForces ^= forceNames::boundary;
				glutPostRedisplay();
            break;
			case '7':
				showForces ^= forceNames::desired;
				glutPostRedisplay();
            break;
			case '8':
				showForces ^= forceNames::final;
				glutPostRedisplay();
            break;
			case '9':
				showForces ^= forceNames::velocity;
				glutPostRedisplay();
            break;
			case '+':
				forceStretchFactor*=2.f;
				glutPostRedisplay();
            break;
			case '-':
				forceStretchFactor/=2.f;
				if (forceStretchFactor < 1.f) forceStretchFactor=1.0f;
				glutPostRedisplay();
            break;
			case ']':
				velocityStretchFactor*=2.f;
				glutPostRedisplay();
            break;
			case '[':
				velocityStretchFactor/=2.f;
				if (velocityStretchFactor < 1.f) velocityStretchFactor=1.0f;
				glutPostRedisplay();
            break;
			case '0':
				showForces ^= (int(1) << 25);
				glutPostRedisplay();
            break;

			case 'k': //schedule a cell for removal
				GetSceneViewSize(windowSizeX,windowSizeY, xVisF,xVisT,yVisF,yVisT);
				sx=(float)mx              /(float)windowSizeX * (xVisT-xVisF) + xVisF;
				sy=(float)(windowSizeY-my)/(float)windowSizeY * (yVisT-yVisF) + yVisF;

				for (c=agents.begin(); c != agents.end(); c++)
					if ((*c)->IsPointInCell(Vector3d<float>(sx,sy,0)))
						(*c)->shouldDie=true;

				glutPostRedisplay();
				break;
    }
}


// called when a special key is pressed
void Skeyboard(int key, int mx, int my)
{
	//required for calculating the mouse position within the scene coords
	float xVisF, yVisF;
	float xVisT, yVisT;
	float sx,sy;

	//required for sweeping the cell list
	std::list<Cell*>::iterator c;

    switch (key) {
			case GLUT_KEY_LEFT:
				//rotate cell left by 30deg
				GetSceneViewSize(windowSizeX,windowSizeY, xVisF,xVisT,yVisF,yVisT);
				sx=(float)mx              /(float)windowSizeX * (xVisT-xVisF) + xVisF;
				sy=(float)(windowSizeY-my)/(float)windowSizeY * (yVisT-yVisF) + yVisF;

				for (c=agents.begin(); c != agents.end(); c++)
					if ((*c)->IsPointInCell(Vector3d<float>(sx,sy,0)))
					{
						(*c)->desiredDirection += PI/6.f;
						if ((*c)->desiredDirection > PI) 
							(*c)->desiredDirection -= 2.f*PI;
					}
				break;
			case GLUT_KEY_RIGHT:
				//rotate cell left by 30deg
				GetSceneViewSize(windowSizeX,windowSizeY, xVisF,xVisT,yVisF,yVisT);
				sx=(float)mx              /(float)windowSizeX * (xVisT-xVisF) + xVisF;
				sy=(float)(windowSizeY-my)/(float)windowSizeY * (yVisT-yVisF) + yVisF;

				for (c=agents.begin(); c != agents.end(); c++)
					if ((*c)->IsPointInCell(Vector3d<float>(sx,sy,0)))
					{
						(*c)->desiredDirection -= PI/6.f;
						if ((*c)->desiredDirection < -PI) 
							(*c)->desiredDirection += 2.f*PI;
					}
				break;
    }
}


/// called when "real" menu item is activated
void menu(int key)
{
	switch (key)
	{
	case -1: break;
	default:
		keyboard((unsigned char)key,0,0);
	}
}


// called when the window is reshaped, arguments are the new window size
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	 //remember the new window size...
	 windowSizeX=w; windowSizeY=h;

	 //what will be the final view port
    float xVisF, yVisF;
    float xVisT, yVisT;

	 GetSceneViewSize(w,h,xVisF,xVisT,yVisF,yVisT);
    
    glOrtho(xVisF, xVisT, yVisF, yVisT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}


//------------------------

bool CheckShader(GLuint n_shader_object, const char *p_s_shader_name)
{
	int n_compiled;
	glGetShaderiv(n_shader_object, GL_COMPILE_STATUS, &n_compiled);
	int n_log_length;
	glGetShaderiv(n_shader_object, GL_INFO_LOG_LENGTH, &n_log_length);
	if(n_log_length > 1) {
		char *p_s_info_log = new char[n_log_length + 1];
		glGetShaderInfoLog(n_shader_object, n_log_length, &n_log_length, p_s_info_log);
		REPORT("GLSL compiler (" << p_s_shader_name << "): " << p_s_info_log);
		delete[] p_s_info_log;
	}
	return n_compiled == GL_TRUE;
}

bool CheckProgram(GLuint n_program_object, const char *p_s_program_name)
{
	int n_linked;
	glGetProgramiv(n_program_object, GL_LINK_STATUS, &n_linked);
	int n_length;
	glGetProgramiv(n_program_object, GL_INFO_LOG_LENGTH, &n_length);
	if(n_length > 1) {
		char *p_s_info_log = new char[n_length + 1];
		glGetProgramInfoLog(n_program_object, n_length, &n_length, p_s_info_log);
		REPORT("GLSL linker (" << p_s_program_name << "): " << p_s_info_log);
		delete[] p_s_info_log;
	}
	return n_linked == GL_TRUE;
}


bool initializeGL(void)
{
	 int Argc=1;
	 char msg[]="TRAgen: A Tool for Generation of Synthetic Time-Lapse Image Sequences of Living Cells";
	 char *Argv[10];
	 Argv[0]=msg;

    glutInit(&Argc, Argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(int(params.imgSizeX), int(params.imgSizeY)); 
    glutInitWindowPosition(100, 100);
    glutCreateWindow(Argv[0]);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);       //black background for Fluo
    //glClearColor(0.62f, 0.62f, 0.62f, 0.0f);  //gray background for PhC
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);

// Initialize GLEW
//glewExperimental = GL_TRUE;
glewInit();

///FRAMEBUFFER
glGenFramebuffers(1, &frameBuffer);
glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
//
glGenRenderbuffers(2,rb);
glBindRenderbuffer(GL_RENDERBUFFER, rb[0]);
glRenderbufferStorage(GL_RENDERBUFFER,GL_RGB,GLsizei(params.imgSizeX),GLsizei(params.imgSizeY));
glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,rb[0]);
glBindRenderbuffer(GL_RENDERBUFFER, rb[1]);
glRenderbufferStorage(GL_RENDERBUFFER,GL_R32F,GLsizei(params.imgSizeX),GLsizei(params.imgSizeY));
glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_RENDERBUFFER,rb[1]);
//
//std::cout << "FB status is " << (unsigned int)glCheckFramebufferStatus(GL_FRAMEBUFFER) << "\n";
if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	REPORT("FB is NOT ready. BAD.");

const GLenum enabledAttachments[2]={GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};
glDrawBuffers(2,enabledAttachments);


//backup the original shader-rendering program
GLint tmp;
glGetIntegerv(GL_CURRENT_PROGRAM,&tmp);
defaultProgram=tmp;

// setup my own special rendering program
// Shader sources
const GLchar* vertexSource150 =
    "#version 150 core\n"
    "in vec2 position;\n"
    "in vec2 texcoord;\n"
    "in float color;\n"
    "out vec2 Texcoord;\n"
    "out float Color;\n"
    "void main() {\n"
    "   Texcoord = texcoord;\n"
    "   Color = color;\n"
    "   gl_Position = vec4(position, 0.0, 1.0);\n"
    "}";
const GLchar* fragmentSource150 =
    "#version 150 core\n"
    "in vec2 Texcoord;\n"
    "in float Color;\n"
    "out vec4 outTexture;\n"
    "out vec4 outMaskID;\n"
    "uniform sampler2D tex;\n"
    "void main() {\n"
    "   outTexture = texture(tex,Texcoord);\n"
	 "   outMaskID = vec4(Color,0.0,0.0,1.0);\n"
    "}";
const GLchar* vertexSource300 =
    "#version 300 es\n"
    "in vec2 position;\n"
    "in vec2 texcoord;\n"
    "in float color;\n"
    "out vec2 Texcoord;\n"
    "out float Color;\n"
    "void main() {\n"
    "   Texcoord = texcoord;\n"
    "   Color = color;\n"
    "   gl_Position = vec4(position, 0.0, 1.0);\n"
    "}";
const GLchar* fragmentSource300 =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 Texcoord;\n"
    "in float Color;\n"
    "out vec4 outTexture;\n"
    "out vec4 outMaskID;\n"
    "uniform sampler2D tex;\n"
    "void main() {\n"
    "   outTexture = texture(tex,Texcoord);\n"
	 "   outMaskID = vec4(Color,0.0,0.0,1.0);\n"
    "}";

//will tell the status of compilation...
bool compileStatusSuccess=true;

// setup my own special rendering program;
// actually we gonna try to compile two versions of the programs
//
// Create and compile the vertex shader
GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertexShader, 1, &vertexSource150, NULL);
glCompileShader(vertexShader);
compileStatusSuccess= CheckShader(vertexShader, "vertex shader");

// Create and compile the fragment shader
GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
if (compileStatusSuccess)
{
	//first part succeeded, move on to the second part
	glShaderSource(fragmentShader, 1, &fragmentSource150, NULL);
	glCompileShader(fragmentShader);
	compileStatusSuccess=CheckShader(fragmentShader, "fragment shader");
}

if (!compileStatusSuccess)
{
	REPORT("GLSL 1.50 was not accepted, trying 3.00 ES....");

	//hmm, let's try the 3.00 ES version of the GLSL code
	glShaderSource(vertexShader, 1, &vertexSource300, NULL);
	glCompileShader(vertexShader);
	compileStatusSuccess= CheckShader(vertexShader, "vertex shader");

	if (!compileStatusSuccess) return (false);

	//first part succeeded, move on to the second part
	glShaderSource(fragmentShader, 1, &fragmentSource300, NULL);
	glCompileShader(fragmentShader);
	compileStatusSuccess=CheckShader(fragmentShader, "fragment shader");

	if (!compileStatusSuccess) return (false);
}

// Link the vertex and fragment shader into a shader program
shaderProgram = glCreateProgram();
glAttachShader(shaderProgram, vertexShader);
glAttachShader(shaderProgram, fragmentShader);
glBindFragDataLocation(shaderProgram, 0, "outTexture");
glBindFragDataLocation(shaderProgram, 1, "outMaskID");
glLinkProgram(shaderProgram);
if(!CheckProgram(shaderProgram, "shader program")) return(false);

//remove unneccessary auxiliary products
glDeleteShader(fragmentShader);
glDeleteShader(vertexShader);

posAttrib = glGetAttribLocation(shaderProgram, "position");
texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
colAttrib = glGetAttribLocation(shaderProgram, "color");


//setup to use the Texture unit 0
glActiveTexture(GL_TEXTURE0);



//---- start: here, load the textures ----

	//"load" the default texture anyway
	GLfloat defaultTexture[] = {
		 1.0f, 1.0f, 1.0f,   0.5f, 0.5f, 0.5f,
		 0.3f, 0.3f, 0.3f,   0.8f, 0.8f, 0.8f
	};
	texTotal=1;

	std::list<Cell*>::iterator iter=agents.begin();
#ifdef READ_TEXTURES
	//okay, go over all agents to find out what texture files to read;
	//remeber the filenames together with lists of pointers on cells
	std::map<std::string , std::list<Cell*> > tm;
	while (iter != agents.end())
	{
		//construct string from the filename
		std::string fn((*iter)->VAO_texFN);

		tm[fn].push_back(*iter);
		iter++;
	}

	//now, that unique'ed list of texture filename is compiled,
	//let's remove those that we actually cannot open
	std::map<std::string , std::list<Cell*> >::iterator tm_iterW,tm_iterE;
	tm_iterW=tm.begin();
	while (tm_iterW != tm.end())
	{
		REPORT_NOENDL("should read texture file: " << tm_iterW->first.c_str());

		std::ifstream tfn(tm_iterW->first.c_str());
		if (!tfn.is_open())
		{
			//this file cannot be opened, remove the whole map item
			tm_iterE=tm_iterW;
			tm_iterW++;

			tm.erase(tm_iterE);
			std::cout << " - NOK\n";
		}
		else
		{
			//move on the next filename
			tm_iterW++;
			tfn.close();
			std::cout << " -  OK\n";
		}
	}

	//add what has remained...
	texTotal+=int(tm.size());
#endif

	glGenTextures(texTotal, tex);

	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, defaultTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_LINEAR);

#ifdef READ_TEXTURES
	//now, go over the map (which has only unique successful filenames)
	//and load the textures
	int texCurrent=1;
	tm_iterW=tm.begin();
	while (tm_iterW != tm.end())
	{
		//reads image from a given file
		i3d::Image3d<i3d::GRAY16> TOimg(tm_iterW->first.c_str()); //Texture Orig
		float* texData=new float[TOimg.GetImageSize()*3];
		int texDataoff=0;
		for (size_t TOy=0; TOy < TOimg.GetSizeY(); ++TOy)
		for (size_t TOx=0; TOx < TOimg.GetSizeX(); ++TOx)
		{
			texData[texDataoff++]=float(TOimg.GetVoxel(TOx,TOy,0))/255.f;
			texData[texDataoff++]=float(TOimg.GetVoxel(TOx,TOy,0))/255.f;
			texData[texDataoff++]=float(TOimg.GetVoxel(TOx,TOy,0))/255.f;
		}

		glBindTexture(GL_TEXTURE_2D, tex[texCurrent]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
						 GLsizei(TOimg.GetSizeX()), GLsizei(TOimg.GetSizeY()), 
						 0, GL_RGB, GL_FLOAT, texData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		//now tell associated cells what is the ID of this texture
		iter=tm_iterW->second.begin();
		while (iter != tm_iterW->second.end())
		{
			(*iter)->VAO_texID=texCurrent;
			iter++;
		}

		texCurrent++;
		tm_iterW++;

		delete texData;
	}
#endif
//----  stop: here, load the textures ----


	//have all the cells their OpenGl stuff initiated (now that OpenGL is finally initiated)
	iter=agents.begin();
	while (iter != agents.end())
	{
		(*iter)->VAO_initStuff();
		iter++;
	}

	//create all the helping menus...
	startMenu=glutCreateMenu(menu);
	glutAddMenuEntry("The application is intended to be run from console/terminal",-1);
	glutAddMenuEntry("(command line in Windows OS) because it communicates all",-1);
	glutAddMenuEntry("auxiliary messages there (not in the GUI).",-1);
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("The application accepts one optional parameter, which is",-1);
	glutAddMenuEntry("the number of cells at the beginning of the simulation.",-1);
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("If one wants to use her own cells, the cell configuration",-1);
	glutAddMenuEntry("files has to be changed. See the cells/format.txt file for",-1);
	glutAddMenuEntry("description of the format. It is also possible to use the",-1);
	glutAddMenuEntry("cellExtractor programme to prepare cell configuration files",-1);
	glutAddMenuEntry("by extracting images of real cells. For tutorial, refer to",-1);
	glutAddMenuEntry("http://cbia.fi.muni.cz/projects/tragen.html",-1);
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("To change other simulation parameters, one has to edit the",-1);
	glutAddMenuEntry("source code (mainly the src/main.cpp file) and recompile.",-1);

	controlsMenu=glutCreateMenu(menu);
	glutAddMenuEntry("The application is controlled with single keys and mouse.",-1);
	glutAddMenuEntry("This menu summarizes all keys the application responds to.",-1);
	glutAddMenuEntry("Most menu items actually do their job by clicking them.",-1);
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("The application is intended to be run from console (command line)",-1);
	glutAddMenuEntry("because the stats are reported only there (not in the GUI).",-1);
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("'h' prints this help (on a console!)",(int)('h'));
	glutAddMenuEntry("'d' prints what features are displayed (on a console!)",(int)('d'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("SPACEBAR (step 1 min) and 'z' (step 10 min) run/pause simulation",-1);
	glutAddMenuEntry("'n' pause and step simulation",(int)('n'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("'r' reset screen size",(int)('r'));
	glutAddMenuEntry("'T' toggle display of the time stamp",(int)('T'));
	glutAddMenuEntry("'C' toggle display of the number of cells",(int)('C'));
	glutAddMenuEntry("'t' toggle display between outline or texture mode",(int)('t'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("'c' show/hide approximating circles",(int)('c'));
	glutAddMenuEntry("'p' show/hide current boundary points",(int)('p'));
	glutAddMenuEntry("'l' lists current boundary points of selected cells",(int)('l'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("'1'--'9' show/hide particular force",-1);
	glutAddMenuEntry("'-','+' decrease,increase drawn force vectors",-1);
	glutAddMenuEntry("'[',']' decrease,increase drawn velocity vectors",-1);
	glutAddMenuEntry("'0' toggle display of forces between selected or all cells",(int)('0'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("'i'/'I' inspects state/tells ID of cell under mouse cursor",-1);
	glutAddMenuEntry("'s' select/deselect cell under mouse cursor",(int)('s'));
	glutAddMenuEntry("'S' deselect all selected cells",(int)('S'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("'k' kills a cell under mouse cursor",(int)('k'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("LEFT,RIGHT KEY turn cell under cursor 30deg left,right",-1);

	displayMenu=glutCreateMenu(menu);
	glutAddMenuEntry("Currently displayed features are listed on the console ('d').",-1);
	glutAddMenuEntry("The application can, however, display:",-1);
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("- Approximating circles around current cell positions ('c')",int('c'));
	glutAddMenuEntry("- Boundary points at the current cell positions ('p')",int('p'));
	glutAddMenuEntry("- Selected force vectors and their stretch factor",-1);
	glutAddMenuEntry("- Velocity vector and its stretch factor",-1);
	glutAddMenuEntry("- Vectors of all or only selected cells ('0')",int('0'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("Colour legend of the vectors and keys that show/hide them:",-1);
	glutAddMenuEntry("'1': repulsive force = gray",int('1'));
	glutAddMenuEntry("'2': attraction force = red",int('2'));
	glutAddMenuEntry("'3': body Ph1 force = white",int('3'));
	glutAddMenuEntry("'4': sliding Ph2 force = green",int('4'));
	glutAddMenuEntry("'5': friction force = blue",int('5'));
	glutAddMenuEntry("'6': boundary force = dark green",int('6'));
	glutAddMenuEntry("'7': desired force = magenta",int('7'));
	glutAddMenuEntry("'8': resulting force = cyan",int('8'));
	glutAddMenuEntry("'9': current velocity = dark magenta",int('9'));
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("Colour legend of the cell boundary:",-1);
	glutAddMenuEntry("Yellow to red boundary = cell during interphase ",-1);
	glutAddMenuEntry("Light blue to dark blue boundary = cell during mitosis",-1);
	glutAddMenuEntry("Boundary darkens proportionally to the greatest",-1);
	glutAddMenuEntry("individual force acting on the particular cell.",-1);

	aboutMenu=glutCreateMenu(menu);
	glutAddMenuEntry("TRAgen: GNU GPL tracks generator for biomedical tracking",-1);
	glutAddMenuEntry("Authors: Vladimir Ulman, Zoltan Oremus, David Svoboda",-1);
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("Copyright  2015  Centre for Biomedical Image Analysis",-1);
	glutAddMenuEntry("http://cbia.fi.muni.cz",-1);
	glutAddMenuEntry("  ",-1);
	glutAddMenuEntry("contact email: ulman@fi.muni.cz",-1);

	mainMenu=glutCreateMenu(menu); //should be global and removed in CloseGL()
	glutAddSubMenu("Invoking TRAgen",startMenu);
	glutAddSubMenu("Controls summary",controlsMenu);
	glutAddSubMenu("Displayed features",displayMenu);
	glutAddSubMenu("About",aboutMenu);
	glutAddMenuEntry("----------------------",-1);
	glutAddMenuEntry("Start/stop simulation,    step 10 min",int('z'));
	glutAddMenuEntry("Start/stop simulation,    step   1 min",int(' '));
	glutAddMenuEntry("One step of simulation, step 0.1 min",int('n'));
	glutAddMenuEntry("----------------------",-1);
	glutAddMenuEntry("Quit (ESC or 'q')",int('q'));

	glutAttachMenu(GLUT_RIGHT_BUTTON);

    //glutMouseFunc(click);
    //glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(Skeyboard);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

	return(true);
}


void loopGL(void)
{
	glutMainLoop();
}


void closeGL(void)
{
	glutDestroyMenu(startMenu);
	glutDestroyMenu(controlsMenu);
	glutDestroyMenu(displayMenu);
	glutDestroyMenu(aboutMenu);
	glutDestroyMenu(mainMenu);

    glDeleteProgram(shaderProgram); //local shader-visualization programme

	 glDeleteTextures(texTotal, tex); //textures

	 glDeleteRenderbuffers(2,rb); //local memory for the Framebuffer
	 glDeleteFramebuffers(1, &frameBuffer); //Framebuffer to render into, off the screen
}
