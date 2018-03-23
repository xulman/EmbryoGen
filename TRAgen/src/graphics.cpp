/*
Basic diffusion and FRAP simulation
 Jonathan Ward and Francois Nedelec, Copyright EMBL Nov. 2007-2009

 
To compile on the mac:
 g++ main.cc -framework GLUT -framework OpenGL -o frap

To compile on Windows using Cygwin:
 g++ main.cc -lopengl32 -lglut32 -o frap
 
To compile on Linux:
 g++ main.cc -lglut -lopengl -o frap
*/

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include "GL/glut.h"
#endif
#include <list>
#include <vector>
#include <iostream>
#include <math.h>

#include "params.h"
#include "agents.h"
#include "simulation.h"

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

///OpenGL window size -> to recalculate from windows coords to scene coords
int windowSizeX=0, windowSizeY=0;

#ifdef DEBUG_CALCDISTANCE2
 //link to debug vectors from agents.cpp
 extern std::vector<Vector3d<float> > pointsToShow;
 extern std::vector<Vector3d<float> > pairsToShow;
#endif


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
		std::cout << "END OF THE SIMULATION HAS BEEN REACHED\n";
		pauseSimulation=1;
	}

	std::cout << "\nnow at time: " << params.currTime
		<< " min; there are currently " << agents.size()
		<< " cells in the system\n";

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


void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    displayEdges();

	//iterate over all cells and display them
	std::list<Cell*>::const_iterator iter=agents.begin();
	while (iter != agents.end())
	{
		(*iter)->DrawCell(showCells);
		(*iter)->DrawForces(showForces,forceStretchFactor,velocityStretchFactor);
		iter++;
	}

#ifdef DEBUG_CALCDISTANCE2
	 displayDistanceDebug();
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

    if ( w * yBound < h * xBound )		//equals to  w/h < x/y
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
	std::cout << "'r' reset screen size; 'h' prints this help; 'd' prints what features are displayed\n";
	std::cout << "'s' select/deselect cell under mouse cursor; 'S' deselect all selected cells\n";
	std::cout << "'l' lists current boundary points of selected cells; 'i' inspects state of cell under mouse cursor\n";
	std::cout << "'f' show/hide previous cell positions; 'F' show/hide current cell positions\n";
	std::cout << "'c' show/hide approximating circles; 'p' show/hide current boundary points\n";
	std::cout << "'1'--'9' show/hide particular force; '-','+' decrease,increase drawn force vectors\n";
	std::cout << "'[',']' decrease,increase drawn velocity vectors; '0' toggle between forces of selected or all cells\n";
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
				glutReshapeWindow(params.sceneSize.x,params.sceneSize.y);
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
				GetSceneViewSize(windowSizeX,windowSizeY, xVisF,xVisT,yVisF,yVisT);
				sx=(float)mx              /(float)windowSizeX * (xVisT-xVisF) + xVisF;
				sy=(float)(windowSizeY-my)/(float)windowSizeY * (yVisT-yVisF) + yVisF;

				for (c=agents.begin(); c != agents.end(); c++)
					if ((*c)->IsPointInCell(Vector3d<float>(sx,sy,0)))
					{
						bool wasSelected=(*c)->isSelected;
						(*c)->isSelected=true;
						(*c)->ReportState();
						(*c)->isSelected=wasSelected;
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


void initializeGL(void)
{
	 int Argc=1;
	 char msg[]="TRAgen: A Tool for Generation of Synthetic Time-Lapse Image Sequences of Living Cells";
	 char *Argv[10];
	 Argv[0]=msg;

    glutInit(&Argc, Argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(600, 600); 
    glutInitWindowPosition(100, 100);
    glutCreateWindow(Argv[0]);

    //glutMouseFunc(click);
    //glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(Skeyboard);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
}
