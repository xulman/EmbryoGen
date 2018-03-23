#ifndef PARAMS_H
#define PARAMS_H

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include "GL/glut.h"
#endif
#include <cmath>


///helper class for the PolarListAgent agents
class PolarPair {
 public:
	PolarPair() { ang = 0.f ; dist = 0.f; }
   PolarPair(const float a, const float d) :
		ang(a), dist(d) {}

	float ang,dist;
};


///helper class to sort lists of this kind
class SortPolarPair {
 public:
	bool operator()(const PolarPair& a, const PolarPair& b) const
	{ return(a.ang < b.ang); }
};


///simply a 3D vector...
template <typename T>
class Vector3d {
 public:
	///the vector data
  	T x,y,z;

	///default constructor...
	Vector3d(void) :
		x(0), y(0), z(0) {}

	///init constructor...
	Vector3d(const T xx,const T yy,const T zz) :
		x(xx), y(yy), z(zz) {}

	///init constructor...
	Vector3d(const T xyz) :
		x(xyz), y(xyz), z(xyz) {}

	///copy constructor...
	Vector3d(const Vector3d<T>& vec)
	{
		this->x=vec.x;
		this->y=vec.y;
		this->z=vec.z;
	}

	Vector3d<T>& operator=(const Vector3d<T>& vec)
	{
		this->x=vec.x;
		this->y=vec.y;
		this->z=vec.z;
		return( *this );
	}

	Vector3d<T>& operator=(const T scalar)
	{
		this->x=scalar;
		this->y=scalar;
		this->z=scalar;
		return( *this );
	}

	Vector3d<T>& operator+=(const Vector3d<T>& vec)
	{
		this->x+=vec.x;
		this->y+=vec.y;
		this->z+=vec.z;
		return( *this );
	}

	Vector3d<T>& operator-=(const Vector3d<T>& vec)
	{
		this->x-=vec.x;
		this->y-=vec.y;
		this->z-=vec.z;
		return( *this );
	}

	Vector3d<T>& operator*=(const T& scal)
	{
		this->x*=scal;
		this->y*=scal;
		this->z*=scal;
		return( *this );
	}

	Vector3d<T>& operator/=(const T& scal)
	{
		this->x/=scal;
		this->y/=scal;
		this->z/=scal;
		return( *this );
	}
};


namespace forceNames {
 enum ForceType {
	repulsive  = 1, //aka social, but anti-social in fact
	attractive = 2, //aka stay together
	body       = 4,
	sliding    = 8,
	friction   = 16,
	boundary   = 32,
	desired    = 64, //not really a force...
	final      = 128,
	velocity   = 256, //not really a force...
	unknown
 };
}
typedef forceNames::ForceType ForceType;


//colours of the vectors:
/**
 * a colour table to tell a colour of given type
 * of a force vector an easy version...
 */
static float ForceColorLUT[30]=
// repulsive     attractive    body
  {.5f,.5f,.5f,  1.f,0.f,0.f,  1.f,1.f,1.f,
// sliding       friction      boundary
   0.f,1.f,0.f,  0.f,0.f,1.f,  0.f,.5f,0.f,
// desired       final         velocity
   1.f,0.f,1.f,  0.f,1.f,1.f,  .5f,0.f,.5f,
// unknown
   0.f,.5f,.5f };
//
//social/repulsive = gray
//attraction = red
//body Ph1 = white
//sliding Ph2 = green
//friction = blue
//boundary = dark green
//desired = magenta
//final/total = cyan
//cur velocity = dark magenta
//unknown = dark cyan
//
//current cell position = yellow
//former cell position = light yellow

//shape of the vectors:
#define CONE_HALFANG	0.5236f		//30 deg
#define CONE_LENGTH 0.15f
//time-saver: params for the rotation matrix
const float cs=cosf(CONE_HALFANG);
const float sn=sinf(CONE_HALFANG);


//a 3D vector placed at a certain coordinate
template <typename T>
class ForceVector3d : public Vector3d<float> {
 public:
	///type of the force (only to find out what colour draw with)
	ForceType type;

	///default constructor...
 	ForceVector3d(void) : Vector3d<T>(), type(forceNames::unknown) {}

	///init constructor...
	ForceVector3d(const T xx,const T yy,const T zz,const ForceType tt) :
			Vector3d<T>(xx,yy,zz), type(tt) {}

	///init constructor...
	ForceVector3d(const T xyz,const ForceType tt) :
			Vector3d<T>(xyz), type(tt) {}

	///init constructor...
	ForceVector3d(const Vector3d<T>& v,const ForceType tt) :
			Vector3d<T>(v), type(tt) {}

	///copy constructor...
	ForceVector3d(const ForceVector3d<T>& vec) : Vector3d<T>(vec.x,vec.y,vec.z)
	{
		this->type=vec.type;
	}

	ForceVector3d<T>& operator=(const Vector3d<T>& vec)
	{
		this->x=vec.x;
		this->y=vec.y;
		this->z=vec.z;
		return( *this );
	}

	ForceVector3d<T>& operator=(const T scalar)
	{
		this->x=scalar;
		this->y=scalar;
		this->z=scalar;
		return( *this );
	}

	///draw this vector at given position = \e base
	void Draw(const Vector3d<T>& base,const float stretch=1.f) const
	{
		//get colour of the force vector
		int LUTindex;
		switch (this->type) {
		case forceNames::repulsive:
			LUTindex=0;
			break;
		case forceNames::attractive:
			LUTindex=3;
			break;
		case forceNames::body:
			LUTindex=6;
			break;
		case forceNames::sliding:
			LUTindex=9;
			break;
		case forceNames::friction:
			LUTindex=12;
			break;
		case forceNames::boundary:
			LUTindex=15;
			break;
		case forceNames::desired:
			LUTindex=18;
			break;
		case forceNames::final:
			LUTindex=21;
			break;
		case forceNames::velocity:
			LUTindex=24;
			break;
		default:
			LUTindex=27;
		}
		glColor4f(	ForceColorLUT[LUTindex+0],
						ForceColorLUT[LUTindex+1],
						ForceColorLUT[LUTindex+2],0.8f );

		//time-saver: coordinate of the vector head
		const float xHead=base.x + stretch*this->x;
		const float yHead=base.y + stretch*this->y;

		glBegin(GL_LINES);
		glVertex2f(base.x,base.y);
		glVertex2f(xHead,yHead);

		glVertex2f(xHead,yHead);
		float nx=stretch*(cs*this->x - sn*this->y);
		float ny=stretch*(sn*this->x + cs*this->y);
		glVertex2f(xHead-CONE_LENGTH*nx,yHead-CONE_LENGTH*ny);

		glVertex2f(xHead,yHead);
		nx=stretch*( cs*this->x + sn*this->y);
		ny=stretch*(-sn*this->x + cs*this->y);
		glVertex2f(xHead-CONE_LENGTH*nx,yHead-CONE_LENGTH*ny);
		glEnd();
	}
};


///returns a-b in radians, function takes care of 2PI periodicity
float SignedAngularDifference(const float& a, const float& b);

///returns |a-b| in radians, function takes care of 2PI periodicity
float AngularDifference(const float& a, const float& b);


/// A datatype enumerating the particular phases of cell cycle
typedef enum
{
	 G1Phase=0,
	 SPhase=1,
	 G2Phase=2,
	 Prophase=3,
	 Metaphase=4,
	 Anaphase=5,
	 Telophase=6,
	 Cytokinesis=7
} ListOfPhases;


//finally, a class that contains all global parameters of the simulation
class ParamsClass {
 public:
 	//the scene, the playground for agents to stay within [um]
	Vector3d<float> sceneOffset;
	Vector3d<float> sceneSize;

	//the outer width between the frame and window border [um]
	Vector3d<float> sceneOuterBorder;

	//colour of the frame around the playground rectangle
	struct {
		float r,g,b;
	} sceneBorderColour;

	//returns true if given position is inside the scene
	bool Include(const Vector3d<float> pos) const
	{
		return (	(pos.x >= sceneOffset.x)
				&& (pos.y >= sceneOffset.y)
				&& (pos.z >= sceneOffset.z)
				&& (pos.x < sceneOffset.x + sceneSize.x)
				&& (pos.y < sceneOffset.y + sceneSize.y)
				&& (pos.z < sceneOffset.z + sceneSize.z) );
	}


	//simulation params
	int numberOfAgents;

	float friendshipDuration;  //[min]
	float maxCellSpeed;        //[um/min]

	float cellCycleLength;      //[min]
	float cellPhaseDuration[8]; //[min]

	/**
	 * Initially, it is initial time of the simulation [min].
	 * But it is incremented as the simulation advances.
	 */
	float currTime;

	//how much to increment the \e currTime [min]
	float incrTime;

	//final time of the simulation [min]
	float stopTime;
};


//some helper macros:
#include <iostream>
#include <string.h>

/// helper macro to unify reports:
#define REPORT(x) std::cout \
   	<< std::string(__FUNCTION__) << "(): " << x << std::endl;

#define PI 3.14159265

//#define DEBUG_CALCDISTANCE2

#endif
