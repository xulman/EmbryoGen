#define _USE_MATH_DEFINES
#include <cmath>
#include <list>
#include <vector>
#include <fstream>
#include <float.h>
#include <algorithm>
#include <i3d/image3d.h>

#include "params.h"
#include "agents.h"
#include "rnd_generators.h"

 #include "simulation.h"

#define DEG_TO_RAD(x) ((x)*PI/180.0f)

///whether to enable a time-lapse cell positions reporting
//#define REPORT_STATS

//link to the global params
extern ParamsClass params;

//link to the list of all agents
extern std::list<Cell*> agents;

using namespace std;

//implementation of a function declared in params.h
///returns a-b in radians, function takes care of 2PI periodicity
float SignedAngularDifference(const float& a, const float& b)
{
	if (sin(a-b) > 0.f)
		return acos(cos(a-b));
	else
		return -acos(cos(a-b));

	//sin() was not always 1 or -1 but often something in between
	//return sin(a-b) * acos(cos(a-b));
}


//implementation of a function declared in params.h
///returns |a-b| in radians, function takes care of 2PI periodicity
float AngularDifference(const float& a, const float& b)
{
	return acos(cos(a-b));
}


//function for dot product of two vectors
float dotProduct(const Vector3d<float>& u, const Vector3d<float>& v)
{
	return u.x*v.x+u.y*v.y;
}

///calculates addition of two vector: \e vecA + \e vecB
template <typename T>
Vector3d<T> operator+(const Vector3d<T>& vecA, const Vector3d<T>& vecB)
{
	Vector3d<T> res(vecA);
	res+=vecB;
	return (res);
}

///calculates difference of two vector: \e vecA - \e vecB
template <typename T>
Vector3d<T> operator-(const Vector3d<T>& vecA, const Vector3d<T>& vecB)
{
	Vector3d<T> res(vecA);
	res-=vecB;
	return (res);
}

int GetNewCellID(void) 
{
	static int lastCellID=0;
	return (++lastCellID);
}

void Cell::InitialSettings(bool G1start)
{
	 //we're in constructor, no dying yet
	 this->shouldDie=false;

	 //my new ID
	 this->ID=GetNewCellID();

	 //OTHER STUFF

	 //current cell velocity and others:
	 this->force=0.f;
	 this->force.type=forceNames::final;
	 this->listOfForces.reserve(50);
	 this->acceleration=0.f;
    this->velocity=0.f;
	 this->lastEasyForcesTime=params.currTime;

	 //social force:
	 //social force from all in the vicinity
    this->lambda=1.0f;
	 //starting to build up within a distance of 1um
	 //zero distance = force of 0.2N
	 //-0.2um penetration = force of 3N
    this->A=0.1f;
    this->B=0.6f;

	 //attraction force:
	 //0.05N irrelevant of distance
	 //but times the number of "colliding" points
	 //(these can be up to 20 rendering total force of 1N)
	 this->C=0.02f;
	 this->delta_f=0.15f;

	 //body and sliding friction forces:
	 this->k=0.2f;
	 this->delta_o=0.05f;
	 this->kappa=0.3f;

    this->settleTime=GetRandomUniform(4.,10.);
    this->persistenceTime=GetRandomUniform(10.,20.);

	 //desired direction and speed of movement
	 if (!G1start)
	 {
		 //direction and speed:
		 this->desiredDirection=orientation.ang;
		 this->desiredSpeed=GetRandomUniform(0.,params.maxCellSpeed);

		 this->directionChangeInterval=this->persistenceTime;
		 this->directionChangeCounter=0.f;
		 this->directionChangeLastAngle=0.f;
	 }

	//cell development metadata
	float duration;
	if (G1start)
	{
		//start after mitosis, i.e. in the middle of simulation
		curPhase=G1Phase;
		//duration: this is a code from rotatePhase()
		duration=GetRandomGauss(params.cellPhaseDuration[curPhase],
							0.06f*params.cellPhaseDuration[curPhase]);
							//allows to shorten/prolong duration up to 18%
	}
	else
	{
		//start with the start of the simulation
		curPhase=G2Phase;
		//duration=GetRandomUniform(0.f,params.cellCycleLength);
		int flipCoin=(int) floorf(GetRandomUniform(0.f,4.99f));
		switch (flipCoin)
		{
		case 0:
		case 3:
			duration=GetRandomGauss(params.cellCycleLength*0.25f, //mean
											params.cellCycleLength/7.f); //sigma
			break;
		case 1:
			duration=GetRandomGauss(params.cellCycleLength*0.5f, //mean
											params.cellCycleLength/9.f); //sigma
			break;
		case 2:
			duration=GetRandomGauss(params.cellCycleLength*0.7f, //mean
											params.cellCycleLength/10.f); //sigma
			break;
		default:
			duration=GetRandomGauss(params.cellCycleLength*0.86f, //mean
											params.cellCycleLength/10.f); //sigma
			break;
		}
		if (duration < 0) duration=1.0f;
	}

	if (duration < 0.01f)
	{
		REPORT("NEGATIVE DURATION (" << duration << ") for cell ID=" << this->ID << "!");
		duration=1.f;
	}
	lastPhaseChange=params.currTime;
	nextPhaseChange=params.currTime+duration;
	phaseProgress=0.f;

	 //reset settings for the fake-rotation stuff programmed by Zolo
    this->prog=0;
    this->DirChCount=0;
    this->rotate=false;


	if (!G1start) initializeG2Phase(duration);
	//else G1 will be initialized explicitly from mother
}


Cell::Cell(const Cell& buddy,
		const float R, const float G, const float B) :
			pos(0.f), orientation(0.f,1.f), bp(), former_bp(),
			weight(1.f), initial_bp(), listOfForces(),
			isSelected(false), listOfFriends()
{
	colour.r=R;
	colour.g=G;
	colour.b=B;
	isSelected=buddy.isSelected;

	//now copy the buddy's shape
	pos=buddy.pos;
	former_pos=buddy.pos; //but leave the former_bp list empty
	orientation.ang=buddy.orientation.ang;
	orientation.dist=buddy.orientation.dist;
	//bp=buddy.bp;	//will fill mother of this cell
	outerRadius=buddy.outerRadius;

	weight=buddy.weight;
	initial_bp=buddy.initial_bp;
	initial_orientation.ang =buddy.initial_orientation.ang;
	initial_orientation.dist=buddy.initial_orientation.dist;
	initial_weight=buddy.initial_weight;
	//persistenceTime and settleTime is set in the InitialSettings()

	listOfFriends=buddy.listOfFriends;

	//duplicate some movement stuff
	desiredDirection=buddy.desiredDirection;
	desiredSpeed=buddy.desiredSpeed;
	directionChangeInterval=buddy.directionChangeInterval;
	directionChangeCounter=buddy.directionChangeCounter;
	directionChangeLastAngle=buddy.directionChangeLastAngle;

	//to yield some differences from mother
	InitialSettings(true);

	//keep the original counter
	lastEasyForcesTime=buddy.lastEasyForcesTime;
}


void Cell::calculateNewPosition(const Vector3d<float>& translation,float rotation)
{
	//update: move
	pos+=translation;
}


/*
void Cell::DrawCell(const int showCell) const
{
	if (showCell & 2)
	{
		//set the drawing colour
		glColor4f(0.5f*colour.r,0.5f*colour.g,0.5f*colour.b, 0.8f);

		//draw the agent/cell itself at the former position (if it exists)
		glBegin(GL_LINE_LOOP); //GL_POLYGON must be convex, cells are not
		std::list<PolarPair>::const_iterator p=former_bp.begin();
		while (p != former_bp.end())
		{
			glVertex2f(pos.x+(*p).dist*cos((*p).ang),
						  pos.y+(*p).dist*sin((*p).ang));
			p++;
		}
		glEnd();
	}

	if (showCell & 1)
	{
		//set the drawing colour
		glColor4f(colour.r,colour.g,colour.b, 0.8f);

		//draw the agent/cell itself at the current position
		glBegin(GL_LINE_LOOP); //GL_POLYGON must be convex, cells are not
		std::list<PolarPair>::const_iterator p=bp.begin();
		while (p != bp.end())
		{
			glVertex2f(pos.x+(*p).dist*cos((*p).ang),
						  pos.y+(*p).dist*sin((*p).ang));
			p++;
		}
		glEnd();

		//this->ListBPs();
		//this->ReportState();

		//draw its orientation vector
		glBegin(GL_LINES);
		glVertex2f(pos.x,pos.y);
		glVertex2f(pos.x+orientation.dist*cos(orientation.ang),
					  pos.y+orientation.dist*sin(orientation.ang));
		glEnd();

		//should also draw the circumscribed circle?
		if (showCell & 4)
		{
			glBegin(GL_LINE_LOOP);
			for (float a=0; a < 2*PI; a+=PI/6)
				glVertex2f(pos.x + outerRadius*cos(a),
				           pos.y + outerRadius*sin(a));
			glEnd();
		}

		//should also draw the boundary points?
		if (showCell & 8)
		{
			glPointSize(3.f);
			glBegin(GL_POINTS);
			p=bp.begin();
			while (p != bp.end())
			{
				glVertex2f(pos.x+(*p).dist*cos((*p).ang),
							  pos.y+(*p).dist*sin((*p).ang));
				p++;
			}
			glEnd();
		}

		//draw outline of the cell if desired
		if (isSelected)
		{
			glColor4f(0.9f,0.9f,0.9f, 0.8f);
			glBegin(GL_LINE_LOOP);
			p=bp.begin();
			while (p != bp.end())
			{
				glVertex2f(pos.x+0.85f*(*p).dist*cos((*p).ang),
							  pos.y+0.85f*(*p).dist*sin((*p).ang));
				p++;
			}
			glEnd();
		}
	}
}


void Cell::DrawForces(const int whichForces,const float stretchF,
                      const float stretchV) const
{
	const int whichF = ((whichForces & (int(1) << 25)) && (!isSelected))?
	                   0 : whichForces;

	//draw currently acting forces...
	std::vector<ForceVector3d<float> >::const_iterator f_iter=listOfForces.begin();
	while (f_iter != listOfForces.end())
	{
		if (whichF & f_iter->type) f_iter->Draw(former_pos,stretchF);
		++f_iter;
	}

	//...and their sum
	if (whichF & forceNames::final) this->force.Draw(former_pos,stretchF);

	//also current velocity
	if (whichF & forceNames::velocity)
	{
		ForceVector3d<float> vel(this->velocity,forceNames::velocity);
		vel.Draw(former_pos,stretchV);
	}
}
*/



void Cell::RasterInto(i3d::Image3d<i3d::GRAY8>& img) const
{
	const size_t xC = size_t(pos.x * params.imgResX);
	const size_t yC = size_t(pos.y * params.imgResY);
	const size_t zC = size_t(pos.z * params.imgResZ);
	const size_t xR = size_t(outerRadius * params.imgResX  +0.5f);
	const size_t yR = size_t(outerRadius * params.imgResY  +0.5f);
	const size_t zR = size_t(outerRadius * params.imgResZ  +0.5f);

	//determine bounds for the ball
	const size_t x_min = (xC > xR)? xC-xR : 0;
	const size_t x_max = (xC+xR < img.GetSizeX())? xC+xR : img.GetSizeX();
	const size_t y_min = (yC > yR)? yC-yR : 0;
	const size_t y_max = (yC+yR < img.GetSizeY())? yC+yR : img.GetSizeY();
	const size_t z_min = (zC > zR)? zC-zR : 0;
	const size_t z_max = (zC+zR < img.GetSizeZ())? zC+zR : img.GetSizeZ();

	//sweep the bounds and draw the ball
	const float R2 = outerRadius*outerRadius;
	for (size_t z = z_min; z <= z_max; ++z)
	{
		const float dz = (z-zC) * (z-zC) / (params.imgResZ * params.imgResZ);
		for (size_t y = y_min; y <= y_max; ++y)
		{
			const float dy = (y-yC) * (y-yC) / (params.imgResY * params.imgResY);
			for (size_t x = x_min; x <= x_max; ++x)
			{
				const float dx = (x-xC) * (x-xC) / (params.imgResX * params.imgResX);
				if (dx+dy+dz <= R2)
					img.SetVoxel(x,y,z,(i3d::GRAY8)ID);
			}
		}
	}
}

void Cell::DrawInto(DisplayUnit& ds) const
{
	ds.DrawPoint(ID,pos,outerRadius);
	ds.DrawVector(ID,pos,acceleration);
}



void Cell::ListBPs(void) const
{
	if (!isSelected) return;

	REPORT("cell ID=" << this->ID << ":");
	std::cout << "\n   centre at [" << pos.x << "," << pos.y << "], ";
	std::cout << "orientation at " << orientation.ang << " (dist=" << orientation.dist << "), ";
	REPORT("BPs polar coords (ang,dist):");

	std::list<PolarPair>::const_iterator p=bp.begin();
	int cnt=0;
	while (p != bp.end())
	{
		std::cout << "(" << p->ang << "," << p->dist << "), ";
		if (cnt % 5 == 4) { std::cout << "\n"; }
		++p; ++cnt;
	}
	if (cnt % 5 != 0) { std::cout << "\n"; }
}


void Cell::ReportState(void) const
{
#ifdef REPORT_STATS
		char fn[50];
		int time=floor(params.currTime);
		sprintf(fn,"T %03d",time);
		std::cout << "ID " << this->ID << " " << fn
		          << " POSX " << this->pos.x << " POSY " << this->pos.y << "\n";
#endif

	if (!isSelected) return;

	REPORT("---------- cell ID=" << this->ID << " ----------");
	std::cout << "current phase=" << curPhase << ", phaseProgress=" << phaseProgress
			<< ", lastChange=" << lastPhaseChange << " -> nextChange=" << nextPhaseChange << "\n";
	std::cout << "centre at [" << pos.x << "," << pos.y << "], ";
	std::cout << "orientation at " << orientation.ang << " (dist=" << orientation.dist << ")\n";
	std::cout << "desiredSpeed=" << desiredSpeed << " um/min,   desiredDirection=" << desiredDirection
			<< " rad, rotateBy=" << rotateBy << " rad\n";
	std::cout << "cur velocity=" << sqrt(velocity.x*velocity.x + velocity.y*velocity.y)
 			<< " um/min, velocity vect=(" << velocity.x << "," << velocity.y
			<< ") um/min, vel. direction=" << atan2(velocity.y, velocity.x) << " rad\n";
	std::cout << "cur force   =" << sqrt(force.x*force.x + force.y*force.y) << " N,  force vect=("
 			<< force.x << "," << force.y << ") um/min^2,  force direction="
			<< atan2(force.y, force.x) << " rad\n\n";

	std::vector<ForceVector3d<float> >::const_iterator f
			=listOfForces.begin();
	while (f != listOfForces.end())
	{
		switch (f->type)
		{
		case forceNames::repulsive:
			std::cout << "repulsive force";
			break;
		case forceNames::attractive:
			std::cout << "attractive force";
			break;
		case forceNames::body:
			std::cout << "body force";
			break;
		case forceNames::sliding:
			std::cout << "sliding force";
			break;
		case forceNames::friction:
			std::cout << "friction force";
			break;
		case forceNames::boundary:
			std::cout << "boundary force";
			break;
		case forceNames::desired:
			std::cout << "desired force";
			break;
		default:
			std::cout << "unknown force";
		}
		std::cout << " = " << sqrt(f->x * f->x  +  f->y * f->y) << " N, vect=("
		          << f->x << "," << f->y << ") N, direction="
					 << atan2(f->y, f->x);
		if (f->type == forceNames::attractive)
			std::cout << " rad, " << sqrt(f->x*f->x + f->y*f->y)/C -1 << " pairs\n";
		else
			std::cout << " rad\n";

		++f;
	}
	std::cout << "\n";
}


float Cell::CalcRadiusDistance(const Cell& buddy)
{
	//distance between centres of cells
	float dist= sqrt( (this->pos.x-buddy.pos.x)*(this->pos.x-buddy.pos.x)
						  +(this->pos.y-buddy.pos.y)*(this->pos.y-buddy.pos.y)
						  +(this->pos.z-buddy.pos.z)*(this->pos.z-buddy.pos.z) );
	dist -= this->outerRadius;
	dist -= buddy.outerRadius;

	return (dist);
}


///converts cell boundary point \e pol w.r.t. centre \e pos to Cartesian coordinate \e cart
void CellPolarToGlobalCartesian(const PolarPair& pol,const Vector3d<float>& pos,
                                Vector3d<float>& cart, const float ext=0.f)
{
	cart.x=pos.x + (pol.dist+ext)*cos(pol.ang);
	cart.y=pos.y + (pol.dist+ext)*sin(pol.ang);
	cart.z=0.f;
}

#ifdef DEBUG_CALCDISTANCE2
 ///a debug list of visited points
 std::vector<Vector3d<float> > pointsToShow;
 ///a debug list of visited pairs (should have even length)
 std::vector<Vector3d<float> > pairsToShow;
#endif

/*
float Cell::CalcDistance(const Cell& buddy)
{
	if (isSelected)
		REPORT("ID " << this->ID << ": dist=" << bdistl << " um to buddy at " << angToBuddy
		         << " rad (ID=" << buddy.ID << ")");
	return(bdistl);
}
*/


bool Cell::IsPointInCell(const Vector3d<float>& coord)
{
/* commented out due some bug, TODO

    float azimut = atan2(coord.y-this->pos.y, coord.x-this->pos.x)*180/PI;
    if (azimut < 0.f)
    {
        azimut+=360;
    }


    //std::cout << "azimut is " << azimut << std::endl;


    //CALULATE HIGHER AND LOWER AZIMUT
    std::list<PolarPair>::const_iterator p=bp.begin();
    PolarPair lower_azimut;
    PolarPair higher_azimut;

    //calculate higher and lower azimut
    while (p != bp.end())
    {
        //std::cout << (*p).ang*180/PI << std::endl;
        if(p==bp.begin() )
        {
            std::list<PolarPair>::const_iterator r=bp.end();
            r--;
            if(((*p).ang*180/PI >= azimut || (*r).ang*180/PI <= azimut))
            {
                lower_azimut.ang = (*p).ang;
                higher_azimut.ang = (*r).ang;
                lower_azimut.dist = (*p).dist;
                higher_azimut.dist = (*r).dist;
            }
            else
            {
                std::list<PolarPair>::const_iterator r=p;
                r++;
                if(((*p).ang*180/PI <= azimut && (*r).ang*180/PI >= azimut))
                {

                    lower_azimut.ang = (*p).ang;
                    higher_azimut.ang = (*r).ang;
                    lower_azimut.dist = (*p).dist;
                    higher_azimut.dist = (*r).dist;

                }
            }

        }
        else
        {
            std::list<PolarPair>::const_iterator r=p;
            r++;
            if(((*p).ang*180/PI <= azimut && (*r).ang*180/PI >= azimut))
            {

                lower_azimut.ang = (*p).ang;
                higher_azimut.ang = (*r).ang;
                lower_azimut.dist = (*p).dist;
                higher_azimut.dist = (*r).dist;
            }
        }

        p++;

    }

    //std::cout << "lower_azimut:" << lower_azimut.ang*180/PI << std::endl;
    //std::cout << "higher_azimut:" << higher_azimut.ang*180/PI << std::endl;

    //get point coordinates, this is really tricky, during the case,
    //when higher_azimut is just under 360 and lower just above 0, so it is basically around the entire cell.
    Vector3d<float> point1_coord(0.f);
    Vector3d<float> point2_coord(0.f);

    if(higher_azimut.ang*180.f/PI-lower_azimut.ang*180.f/PI > 180.f)
    {

        point2_coord.x=pos.x+cos(lower_azimut.ang)*lower_azimut.dist;
        point2_coord.y=pos.y+sin(lower_azimut.ang)*lower_azimut.dist;
        point1_coord.x=pos.x+cos(higher_azimut.ang)*higher_azimut.dist;
        point1_coord.y=pos.y+sin(higher_azimut.ang)*higher_azimut.dist;
    }
    else
    {
        point1_coord.x=pos.x+cos(lower_azimut.ang)*lower_azimut.dist;
        point1_coord.y=pos.y+sin(lower_azimut.ang)*lower_azimut.dist;
        point2_coord.x=pos.x+cos(higher_azimut.ang)*higher_azimut.dist;
        point2_coord.y=pos.y+sin(higher_azimut.ang)*higher_azimut.dist;
    }
    //std::cout << "owners 1. point coordinates: " << point1_coord.x << " " << point1_coord.y << "\n";
    //std::cout << "owners 2. point coordinates: " << point2_coord.x << " " << point2_coord.y << "\n";

    //linear interpolation of distance
    float factor1;
    if(higher_azimut.ang*180.f/PI-lower_azimut.ang*180.f/PI > 180)
        factor1 = lower_azimut.ang*180.f/PI+360.f-higher_azimut.ang*180.f/PI;

    else
        factor1 = higher_azimut.ang*180.f/PI-lower_azimut.ang*180.f/PI;

    float factor2;
    if(azimut < lower_azimut.ang*180.f/PI && azimut < higher_azimut.ang*180.f/PI)
        factor2 = azimut +360.f-higher_azimut.ang*180.f/PI;
    else if(azimut > lower_azimut.ang*180.f/PI && azimut > higher_azimut.ang*180.f/PI)
        factor2 = azimut-higher_azimut.ang*180.f/PI;
    else
        factor2 = azimut-lower_azimut.ang*180.f/PI;



    //std::cout << "factor1= " << factor1 << " factor2= " << factor2 << std::endl;

    float linear_factor = factor2/factor1;
    //std::cout << "linear_factor= " << linear_factor << std::endl;


    float point_x = point1_coord.x+(point2_coord.x-point1_coord.x)*linear_factor;
    float point_y = point1_coord.y+(point2_coord.y-point1_coord.y)*linear_factor;

    //std::cout << "point_x= " << point_x << " point_y= " << point_y << std::endl;

    //priemerny azimut

    float dist_x=point_x-pos.x;
    float dist_y=point_y-pos.y;
    float distance = sqrt(dist_x*dist_x+dist_y*dist_y);

    //std::cout << "distance = " << distance << std::endl;

    //ZISTENIE CI DNU, ALEBO VON
    Vector3d<float> line( coord.x-this->pos.x, coord.y-this->pos.y, 0.f);
    float line_length = sqrt(line.x*line.x+line.y*line.y);
    //std::cout << "line length = " << line_length << std::endl;

	 REPORT("cell at [" << pos.x << "," << pos.y << "]: "
	 	<< ((distance >= line_length)? "inside" : "outside"));

    if(distance >= line_length)
        return true;
    else
        return false;
*/

	const float dist= sqrt( (this->pos.x-coord.x)*(this->pos.x-coord.x)
						+(this->pos.y-coord.y)*(this->pos.y-coord.y) );
	if (dist < this->outerRadius) return true;
	else return false;
}


void Cell::calculateSocialForce(const float ang, const float dist)
{
	//positive distance means no collision - truly a distance between cells;
	//ang is an azimuth towards the buddy

    //first part of the equation
    float temp1x = -1.f * this->A * exp(-dist/this->B) * cos(ang);
    float temp1y = -1.f * this->A * exp(-dist/this->B) * sin(ang);

	 if (this->lambda < 1.f)
	 {
		 //my velocity azimuth
		 const float myAng=std::atan2(this->velocity.y, this->velocity.x);

		 //distance between numbers/angles myAng and ang
		 //float Phi=std::min( double(std::abs(myAng - ang)) , 2.*PI - std::abs(myAng - ang) );
		 const float Phi=AngularDifference(myAng, ang);
		 const float lweight = this->lambda + (1.f-this->lambda)*( (1.f+cos(Phi)) / 2.f);

		 //weight as seen in the second part of the equation
		 temp1x*=lweight;
		 temp1y*=lweight;
	 }

	 listOfForces.push_back( ForceVector3d<float>(temp1x,temp1y,0.f,forceNames::repulsive) );
}


//add attraction forces
void Cell::calculateAttractionForce(const float timeDelta)
{
	//iterate throught the listOfFriends and add corresponding force
	std::list<Cell::myFriend>::iterator fr=listOfFriends.begin();
	while (fr != listOfFriends.end())
	{
		if (fr->buddy->shouldDie == false)
		{
			int intersectPointsNo=0;
			//CalcDistance(*(fr->buddy),intersectPointsNo,this->delta_f);

			//create attractive force
			Vector3d<float> aF(fr->buddy->pos.x - pos.x, fr->buddy->pos.y - pos.y, 0);

			//normalize
			aF/=sqrt(aF.x*aF.x + aF.y*aF.y);

			//make adequately strong while keeping some minimal force
			aF*=(float)(intersectPointsNo+1) * this->C;

			//add the attractive force between me and my buddy
			listOfForces.push_back( ForceVector3d<float>(aF,forceNames::attractive) );

			//decrease timeLeft
			fr->timeLeft -= timeDelta;
		}
		else fr->timeLeft=0.f;

		if (fr->timeLeft <= 0.f) fr=listOfFriends.erase(fr);
		else fr++;
	}
}


//function, returns its parameter if higher or equal to zero, else returns zero
float Theta(const float n) { if(n>=0) return n; else return 0; }


//add body counter-deformation forces
void Cell::calculateBodyForce(const float ang, const float dist)
{
	//negative dist means colliding objects,
	//but we're gonna allow for some overlaps
	float temp1x = -1.f * (this->k * Theta(-dist -this->delta_o) + this->A) * cos(ang);
	float temp1y = -1.f * (this->k * Theta(-dist -this->delta_o) + this->A) * sin(ang);

	//force must point away from the buddy
	listOfForces.push_back( ForceVector3d<float>(temp1x,temp1y,0.f,forceNames::body) );
}


void Cell::calculateSlidingFrictionForce(const float ang, const float dist,
		const Vector3d<float>& buddyVelocity)
{
	 //a vector perpendicular to the cell centres connection:
    Vector3d<float> t(cos(ang - PI/2.0f),sin(ang - PI/2.0f),0.f);

	 //a difference between velocities
    Vector3d<float> deltaV(buddyVelocity.x - this->velocity.x,
	                        buddyVelocity.y - this->velocity.y, 0.f);

    float deltaVt=dotProduct(deltaV,t);

	 t.x *= this->kappa * Theta(-dist) * deltaVt;
    t.y *= this->kappa * Theta(-dist) * deltaVt;

    listOfForces.push_back( ForceVector3d<float>(t,forceNames::sliding) );
}


//calculates boundary force, for an agent goes through 4 walls
void Cell::calculateBoundaryForce(void)
{
	//the purpose of this force is just to keep the agents
	//with the sandbox.... no, special terms are therefore computed,
	//just "exponential" from all boundary walls

	//the boundary force...
	float temp1x=0;
	float temp1y=0;

	//from the right wall
	float dist=this->A * exp((pos.x - params.sceneOffset.x-params.sceneSize.x)/this->B);
	temp1x-=dist;

	//from the left wall
	dist=this->A * exp((params.sceneOffset.x - pos.x)/this->B);
	temp1x+=dist;

	//from the top wall (y is high)
	dist=this->A * exp((pos.y - params.sceneOffset.y-params.sceneSize.y)/this->B);
	temp1y-=dist;

	//from the bottom wall (y is small)
	dist=this->A * exp((params.sceneOffset.y - pos.y)/this->B);
	temp1y+=dist;

	//too far outside? hardly to recover smoothly, pretend it just got lost...
	if ((fabsf(temp1x) > 4.f) || (fabsf(temp1y) > 4.f))
	{ 
		this->shouldDie=true;
		DEBUG_REPORT("DEATH DUE TO OUT OF SANDBOX at time " << params.currTime);
		/* force is "relaxed" = not added onto the list of active forces */
	}
	else
	{
		//force element not further than 3 N from zero 
		if (fabsf(temp1x) > 3.f) temp1x*=(3.f/fabsf(temp1x)); //keeps sign
		if (fabsf(temp1y) > 3.f) temp1y*=(3.f/fabsf(temp1y));

		listOfForces.push_back( ForceVector3d<float>(temp1x,temp1y,0.f,forceNames::boundary) );
	}
}


void Cell::calculateDesiredForce(void)
{
	//inserts two forces:
	//the friction-like force:
	float temp1x=this->velocity.x / this->persistenceTime;
	float temp1y=this->velocity.y / this->persistenceTime;
	//
	//this should actually change the velocity directly, hence it is an deceleration
	//make it a force:
	temp1x*=this->weight;
	temp1y*=this->weight;
	listOfForces.push_back( ForceVector3d<float>(-temp1x,-temp1y,0.f,forceNames::friction) );

	//the force to push the cell towards the desired velocity:
	temp1x=this->desiredSpeed * cos(this->desiredDirection) / this->persistenceTime;
	temp1y=this->desiredSpeed * sin(this->desiredDirection) / this->persistenceTime;
	//
	//this is actually a velocity change again...
	temp1x*=this->weight;
	temp1y*=this->weight;
	listOfForces.push_back( ForceVector3d<float>(temp1x,temp1y,0.f,forceNames::desired) );
}


void Cell::ChangeDirection(const float timeDelta)
{
	directionChangeCounter+=timeDelta;
	if (directionChangeCounter >= directionChangeInterval)
	{
	    //desiredDirection=GetRandomUniform(0,3.14159*2);
		 //ISBI2015 idea:
		 //
		 //1) we should not step into this if-block on a regular basis,
		 //the simulated time passed between two consecutive executions
		 //should be more-or-less random (_roughly_ every simulated, e.g., 5min)
		 //
		 //ok, done here:
		directionChangeInterval=GetRandomUniform(0.8f*persistenceTime,1.5f*persistenceTime);

		//2) this is exactly like it was in the ISBI2015:
		//suggest the magnitude of the movement (totally coherent-less)
		float newSpeed=GetRandomUniform(0.,params.maxCellSpeed);

		//3) the amount and direction of rotation should reflect the last such event
		/*
		note from wikipedia:
		
		For the normal distribution, the values less than one standard deviation
		away from the mean account for 68.27% of the set; while two standard
		deviations from the mean account for 95.45%; and three standard deviations
		account for 99.73%.
		
		Hence, setting mean to +- sigma gives disproportion 15:85 in the set.
		The code should very likely keep the sign/orientation of the rotation.
		*/
		//float rotSigma=0.13f; //initial value, should correspond with the max allowed movement
		float rotSigma=0.23f; //initial value, should correspond with the max allowed movement
		float rotMean=0.f;	 //if no movement before, rotMean will stay with 0.f
		if (directionChangeLastAngle > 0.f) rotMean=rotSigma;
		if (directionChangeLastAngle < 0.f) rotMean=-rotSigma;
		//narrows down the sigma if large movement has been done recently
		//rotSigma*=sqrtf(1.f - 0.95f*desiredSpeed/params.maxCellSpeed);
		directionChangeLastAngle=GetRandomGauss(rotMean,rotSigma);

		if (isSelected) REPORT("new ChangeAngle=" << directionChangeLastAngle
				<< ", mean=" << rotMean << " & sigma=" << rotSigma);
		
		//eventually, dump the rotation disproportionally to the velocity
		directionChangeLastAngle*=std::min(1.25f*sqrtf(newSpeed/params.maxCellSpeed),1.f);
		
		desiredDirection+=directionChangeLastAngle;
		desiredSpeed=newSpeed;

	    directionChangeCounter=0.f;
	}
}


void Cell::rotatePhase(void)
{
	//assumes we know for sure the phase is changing

	//finish whatever is in progress, and set the next phase
	switch (curPhase)
	{
	case G1Phase:
		finalizeG1Phase();
		curPhase=SPhase;
		break;
	case SPhase:
		finalizeSPhase();
		curPhase=G2Phase;
		break;
	case G2Phase:
		finalizeG2Phase();
		curPhase=Prophase;
		break;
	case Prophase:
		finalizeProphase();
		curPhase=Metaphase;
		break;
	case Metaphase:
		finalizeMetaphase();
		curPhase=Anaphase;
		break;
	case Anaphase:
		finalizeAnaphase();
		curPhase=Telophase;
		break;
	case Telophase:
		finalizeTelophase();
		curPhase=Cytokinesis;
		break;
	case Cytokinesis:
		finalizeCytokinesis();
		curPhase=G1Phase;
		break;
	}

	//determine its duration
	float duration=GetRandomGauss(params.cellPhaseDuration[curPhase],
							0.06f*params.cellPhaseDuration[curPhase]);
							//allows to shorten/prolong duration up to 18%

	//update the progress related values
	lastPhaseChange=nextPhaseChange;
	nextPhaseChange+=duration;
	phaseProgress=0.f;

	//and initiate it
	switch (curPhase)
	{
	case G1Phase:
		initializeG1Phase(duration);
		break;
	case SPhase:
		initializeSPhase(duration);
		break;
	case G2Phase:
		initializeG2Phase(duration);
		break;
	case Prophase:
		initializeProphase(duration);
		break;
	case Metaphase:
		initializeMetaphase(duration);
		break;
	case Anaphase:
		initializeAnaphase(duration);
		break;
	case Telophase:
		initializeTelophase(duration);
		break;
	case Cytokinesis:
		initializeCytokinesis(duration);
		break;
	}
}


void Cell::adjustShape(const float timeDelta)
{
	//init the list of forces so that some routine/change
	//may already insert some...
	listOfForces.clear();

        //cycle cell phases
        //(it can go even over several phases if time goes too fast,
        //in that case the finalize* and initialize* functions will
        //be alternated until the proper nextPhaseChange is reached)

        while (params.currTime+timeDelta > nextPhaseChange) rotatePhase();
        phaseProgress=(params.currTime+timeDelta -lastPhaseChange)
                                    / (nextPhaseChange-lastPhaseChange);
        switch (curPhase)
        {
        case G1Phase:
            progressG1Phase();
            break;
        case SPhase:
            progressSPhase();
            break;
        case G2Phase:
            progressG2Phase();
            break;
        case Prophase:
            progressProphase();
            break;
        case Metaphase:
            progressMetaphase();
            break;
        case Anaphase:
            progressAnaphase();
            break;
        case Telophase:
            progressTelophase();
            break;
        case Cytokinesis:
            progressCytokinesis();
            break;
        }

	if (ID == 1)
		listOfForces.push_back(*(new ForceVector3d<float>(0.f,-0.3f,0.f,forceNames::desired)));

	//theoretically, this function may already set some forces...
}


void Cell::calculateForces(const float timeDelta)
{
    //the forces for this run have already been initiated in the Cell::adjustShape()

	//second, process over all buddies
	std::list<Cell*>::const_iterator bddy=agents.begin();
	while (bddy != agents.end())
	{
		//do not evaluate against myself!
		if (*bddy == this) { bddy++; continue; }

		//syntactic short-cut...
		const Cell* buddy=*bddy;

		//estimate roughly the distance between the two cells
		float dist=CalcRadiusDistance(*buddy);

		//are they close to each other?
		if (dist < 1.0f)
		{
			//hmm, we need to update dist with more precise value
			//use the same 'dist'

			//azimuth towards the buddy
			float ang=std::atan2(buddy->pos.y - this->pos.y,
			                     buddy->pos.x - this->pos.x);

			//are they so close that they can "feel" each other?
			if (dist <= this->delta_f)
			{
				//make the buddy my new friend
				std::list<Cell::myFriend>::iterator fr=listOfFriends.begin();
				while ((fr != listOfFriends.end()) && (fr->buddy != buddy)) fr++;

				if (fr != listOfFriends.end())
					//already on the list, update duration
					fr->timeLeft=params.friendshipDuration;
				else
					//new on the list...
					listOfFriends.push_back(Cell::myFriend(buddy,params.friendshipDuration));
			}

			//are they colliding?
			if (dist <= 0.f) //or (intersectPointsNo > 0)
			{
				//yes, collision
				//
				//add emergent physical-contact forces
				calculateBodyForce(ang,dist);
				calculateSlidingFrictionForce(ang,dist,buddy->velocity);
			}
			else
			{
				//no collision
				if (dist < 1.0f)
					//more precise distance claims they are really close to each other
					calculateSocialForce(ang,dist);
			}
		}

		//next, please...
		bddy++;
	}

	//third, also account for my own desire, friends, and boundary...
	//boundary:
	calculateBoundaryForce();

	//friends:
	//calculateAttractionForce(timeDelta);

	//myself:
	calculateDesiredForce();

	//finally, compute the summary force
	force=0.f;

	//and check whether we are not under big stress
	bool bigForcePresent=false;
	float greatestForce=0.f;

	std::vector<ForceVector3d<float> >::const_iterator f_iter=listOfForces.begin();
	while (f_iter != listOfForces.end())
	{
		force.x += f_iter->x;
		force.y += f_iter->y;

		//big force?
		const float mag=sqrt(f_iter->x*f_iter->x + f_iter->y*f_iter->y);
		if ((f_iter->type != forceNames::attractive) && (mag > 0.3f)) bigForcePresent=true;
		if (mag > greatestForce) greatestForce=mag;

		++f_iter;
	}

	/*
	 * seems to me that any cell can get rid of forces exceeding 0.1N within
	 * a period as long as mitosis... if not, die it.
	 */
	//if no big force or we're in mitosis then reset/update the counter;
	//otherwise, if some big force is present continously over a given period,
	//the cell can't stand it anymore and breaks down
	if ((!bigForcePresent) || (curPhase > 2))
	{
		lastEasyForcesTime=params.currTime;
	}
	else if ((params.currTime - lastEasyForcesTime) > (0.05f*params.cellCycleLength))
	{
		shouldDie=true;
		DEBUG_REPORT("DEATH DUE TO EXTENSIVE STRESS at time " << params.currTime);
	}

	if (greatestForce > 0.5f) greatestForce=0.5f;
	//makes the cell more reddish the greater force pushes
	colour.g=1.f - greatestForce/0.5f;
}


void Cell::applyForces(const float timeDelta)
{
	//assumes all forces for the current state have been computed
	//but in fact, we are interested only in the Cell::force
	//
	//note: we are missing a "erratic/chaos term Epsilon"

	//now, translation is a result of forces:
	//F=ma -> a = F/m
	acceleration.x = force.x/weight;
	acceleration.y = force.y/weight;
	
	//v=at
	velocity.x += acceleration.x*timeDelta;
	velocity.y += acceleration.y*timeDelta;

	//|trajectory|=vt
	Vector3d<float> translation;
	translation.x = velocity.x*timeDelta;
	translation.y = velocity.y*timeDelta;

	//backup before applying...
	former_bp=bp;
	former_pos=pos;

	//apply the translation
	calculateNewPosition(translation,0);
}


void Cell::Grow()
{

    std::list<PolarPair>::iterator p=bp.begin();
    std::list<PolarPair>::const_iterator q=initial_bp.begin();

    while(q!=initial_bp.end())
    {


        float tmp_dist = (*q).dist - (*p).dist;
        float help = tmp_dist/((float)grow_iterations-(float)cur_grow_it);
        (*p).dist = (*p).dist + help;
        (*p).ang = (*q).ang;

			//update the search for maximum radius
			if (p->dist > outerRadius) outerRadius=p->dist;

        p++;
        q++;
    }
    cur_grow_it++;
}


void Cell::initializeG1Phase(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");
}

void Cell::initializeSPhase(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");
}

void Cell::initializeG2Phase(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");
}

void Cell::initializeProphase(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");
}

void Cell::initializeMetaphase(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");
}

void Cell::initializeAnaphase(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");
}

void Cell::initializeTelophase(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");
}

void Cell::initializeCytokinesis(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");
}


void Cell::progressG1Phase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::progressSPhase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::progressG2Phase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::progressProphase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::progressMetaphase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::progressAnaphase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::progressTelophase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::progressCytokinesis(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}


void Cell::finalizeG1Phase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::finalizeSPhase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::finalizeG2Phase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::finalizeProphase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::finalizeMetaphase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::finalizeAnaphase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::finalizeTelophase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
}

void Cell::finalizeCytokinesis(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);

	 //I'm gonna split the list in the upperPoint and lowerPoint
	 //but before we need new cell...
	 Cell* cc=new Cell(*this, colour.r,colour.g,colour.b);
	 agents.push_back(cc);
	 //copy const. calls InitialSettings(true) and so the new
	 //cell lacks initializeG1Phase() and init of bp list, and is already in G1Phase
	 //
	 //get the former-mother now-new-daughter cell a new ID


	 int MoID=this->ID;
	 this->ID=GetNewCellID();
	 ReportNewBornDaughters(MoID,this->ID,cc->ID);

	cc->initializeG1Phase(cc->nextPhaseChange - cc->lastPhaseChange);
	//this->initializeG1Phase(duration) will call my caller

	this->pos.x -= 0.8f * this->outerRadius;
	  cc->pos.x += 0.8f *   cc->outerRadius;

	params.numberOfAgents++;
}


//---------------------------------------------------------------------------------------------
void Cell::doCytokinesis(const float progress)
{
	 // Find two (upper and lower) boundary point that will control the contraction 
	 // of the cleavage furrow.
    std::list<PolarPair>::iterator upperPoint, lowerPoint;
	 FindMinorAxis(upperPoint, lowerPoint);

	 // At this moment, the two points occuring at angles PI/2 and 3*PI/2 relatively
	 // to two cell major axis (orientation) are detected. These points however need
	 // not be the points that are sufficiently close to the cell center. In the following
	 // code, we trace some neighbours of these two points to find such candidates that
	 // are located in the nearest possible position to the cell center.
	 std::list<PolarPair>::iterator leftUpperP, rightUpperP, leftLowerP, rightLowerP;
	 const float angleTolerance = DEG_TO_RAD(45.0f);

	 FindNearest(upperPoint, leftUpperP, rightUpperP, angleTolerance);
	 FindNearest(lowerPoint, leftLowerP, rightLowerP, angleTolerance);

    // TODO: mozna ta funkce na vypocet prumeru nebude potreba a budu ji moc smazat
	 // Find out the diameter of the cell measured along the vector 'upperPoint->lowerPoint'
	 // float diameter = GetCellDiameter(*upperPoint, *lowerPoint);
	 /// float radius = diameter/2.0f;

	 // Shrink the cleavage furrow to fit the required progress
	 //upperPoint->dist *= radius*(1.0f - progress);
	 // lowerPoint->dist *= (1.0f - progress);
	 //


	 std::list<PolarPair>::iterator q;
	 size_t level;

	 // Apply cascade deformation on all the neighbouring points to the right of upperPoint
	 q = upperPoint;
	 level = 1; // further from the central point -> lower deformation
	 while (q != rightUpperP)
	 {
		  // std::cout << "pred: " << q->dist << std::endl;
		  q->dist *= (1.0f - progress/(float)level);
		  //std::cout << "po: " << q->dist << std::endl;
		  level *= 2;

		  q++;

		  if (q == bp.end())
				q = bp.begin();
	 }

	 // Apply cascade deformation on all the neighbouring points to the left of upperPoint
	 // and skip this point (it has already been solved before)
	 q = upperPoint;
	 if (q == bp.begin())
		  q = bp.end();

	 q--;

	 level = 1; // further from the central point -> lower deformation
	 while (q != leftUpperP)
	 {
		  q->dist *= (1.0f - progress/(float)level);
		  level *= 2;

		  if (q == bp.begin())
				q = bp.end();

		  q--;
	 }

	 // Apply cascade deformation on all the neighbouring points to the right of lowerPoint
	 q = lowerPoint;
	 level = 1; // further from the central point -> lower deformation
	 while (q != rightLowerP)
	 {
		  // std::cout << "pred: " << q->dist << std::endl;
		  q->dist *= (1.0f - progress/(float)level);
		  // std::cout << "po: " << q->dist << std::endl;
		  level *= 2;

		  q++;

		  if (q == bp.end())
				q = bp.begin();
	 }

	 // Apply cascade deformation on all the neighbouring points to the left of lowerPoint
	 // and skip this point (it has already been solved before)
	 q = lowerPoint;
	 if (q == bp.begin())
		  q = bp.end();

	 q--;

	 level = 1; // further from the central point -> lower deformation
	 while (q != leftLowerP)
	 {
		  q->dist *= (1.0f - progress/(float)level);
		  level *= 2;

		  if (q == bp.begin())
				q = bp.end();

		   q--;
	 }

}

//---------------------------------------------------------------------------------------------
void Cell::FindMinorAxis(std::list<PolarPair>::iterator &upperPoint,
								 std::list<PolarPair>::iterator &lowerPoint)
{
    std::list<PolarPair>::iterator q;
	 upperPoint = bp.begin();
	 lowerPoint = bp.begin();
	 
	 float angleDist, upperDist = FLT_MAX, lowerDist = FLT_MAX;

	 for (q=bp.begin(); q!=bp.end(); q++) // loop through all the boundary point
	 {
		  float currentRelativeAngle = Normalize(q->ang);

		  // is the elevation of current point near to PI/2?
		  // if so - store it as a candidate to upperPoint
		  angleDist = fabs(currentRelativeAngle - PI/2.0f); 
		  if (angleDist < upperDist) 
		  {
				upperDist = angleDist;
				upperPoint = q;
		  }

		  // is the elevation of current point near to 3*PI/2?
		  // if so - store it as a candidate to lowerPoint
		  angleDist = fabs(currentRelativeAngle - 3.0f*PI/2.0f);
		  if (angleDist < lowerDist)
		  {
				lowerDist = angleDist;
				lowerPoint = q;
		  }
	 }
}

//---------------------------------------------------------------------------------------------
void Cell::FindNearest(std::list<PolarPair>::iterator &centralPoint, 
							  std::list<PolarPair>::iterator &leftPoint,
							  std::list<PolarPair>::iterator &rightPoint,
							  float angleTolerance)
{
	 std::list<PolarPair>::iterator candidate = centralPoint; // initial value
	 float dist = FLT_MAX, angleDist;

	 // inspect the left neighbourhood
	 leftPoint = centralPoint;
	 angleDist = 0;

	 while (angleDist < angleTolerance)
	 {
		  // check the angluar distance between central point and currently
		  // inspected point
		  angleDist = fabs(Normalize(centralPoint->ang) - Normalize(leftPoint->ang));

		  // check the distance between the currently inspected point and cell center
		  if (leftPoint->dist < dist) 
		  {
				dist = leftPoint->dist;
				candidate = leftPoint;
		  }

		  // shift yourself to the left 
		  if (leftPoint == bp.begin())
				leftPoint = bp.end();

		  leftPoint--;

	 }

	 // inspect the right neighbourhood
	 rightPoint = centralPoint;
	 angleDist = 0;

	 while (angleDist < angleTolerance)
	 {
		  // check the angluar distance between central point and currently
		  // inspected point
		  angleDist = fabs(Normalize(centralPoint->ang) - Normalize(rightPoint->ang));

		  // check the distance between the currently inspected point and cell center
		  if (rightPoint->dist < dist) 
		  {
				dist = rightPoint->dist;
				candidate = rightPoint;
		  }

		  // shift yourself to the left 
		  rightPoint++;

		  if (rightPoint == bp.end())
				rightPoint = bp.begin();

	 }

	 centralPoint = candidate;
}

//---------------------------------------------------------------------------------------------
float Cell::FindNearest(const float angle,
						std::list<PolarPair>::iterator& p)
{
	std::list<PolarPair>::iterator s=bp.begin();

	//so far the best solution:
	p=s;
	float bestAngle=AngularDifference(s->ang,angle);

	//scan all BPs to find better
	while (s != bp.end())
	{
		float curAngle=AngularDifference(s->ang,angle);
		if (curAngle < bestAngle)
		{
			p=s;
			bestAngle=curAngle;
		}

		++s;
	}

	return (bestAngle);
}

//---------------------------------------------------------------------------------------------
float Cell::Normalize(float angle)
{
	 angle -= this->orientation.ang; // make the angle relative to the origin

	 if (angle < 0.0f) // convert the angle to the range <0; 2*PI>
		  return (angle + 2.0f*PI); 
	 
	 return angle;
}

//---------------------------------------------------------------------------------------------
float Cell::GetCellDiameter(PolarPair &a, PolarPair &b)
{
	 // convert coordinated from polar to cartesian
	 float ax = a.dist * cos(a.ang);
	 float ay = a.dist * sin(a.ang);

	 float bx = b.dist * cos(b.ang);
	 float by = b.dist * sin(b.ang);

	 // build a vector 'v = a->b'
	 float vx = bx - ax;
	 float vy = by - ay;
	 float len = sqrtf(vx*vx + vy*vy);

	 // normalize the vector
	 float nx = vx/len;
	 float ny = vy/len; 

	 // Inspect all the bondary points. Build vectors from the center to these
	 // points and inspect their distance by measuring the projection of these
	 // vector the the vector 'v'. The projection is realized via inner product.

    std::list<PolarPair>::iterator q;
	 float maxPositive = 0;
	 float maxNegative = 0;

	 for (q=bp.begin(); q!=bp.end(); q++) // loop through all the boundary point
	 {
		  float product = q->dist*cos(q->ang)*nx + q->dist*sin(q->ang)*ny;

		  if (maxPositive < product)
				maxPositive = product;

		  if (maxNegative > product)
				maxNegative = product;
	 }

	 // Add the largest distance toward the upper point with thye largest
	 // distance to the lower point.
	 return maxPositive + fabs(maxNegative);
}

void Cell::roundCell(void)
{
    std::list<PolarPair>::iterator p=bp.begin();


	//reset the outerRadius to find now correct value
	outerRadius=0.f;

    float value = sqrt(volume/PI);
    while(p!=bp.end())
    {

        float tmp_dist = value - (*p).dist;
        float help = tmp_dist/(round_cell_duration-round_cell_progress);
        (*p).dist = (*p).dist + help;

            //update the search for maximum radius
            if (p->dist > outerRadius) outerRadius=p->dist;

        p++;

    }
}

void Cell::InitialRotation(float rotation)
{
    //update: rotate
    std::list<PolarPair>::iterator p=bp.begin();
    std::list<PolarPair>::iterator q=rotate_bp.begin();
    while (p != bp.end())
    {
        q->ang = std::fmod(p->ang + rotation, 2.0f*PI);

        /* for detecting of crippled shapes
        if (p->dist < 1) std::cout << "dist=" << p->dist
                                << ", curPhase=" << this->curPhase
                                << ", phaseProgress=" << this->phaseProgress << "\n";
        */
        cout << p->dist << " " << q->dist << endl;
        p++;
        q++;
    }
    orientation.ang = std::fmod(orientation.ang + rotation, 2.0f*PI);


    this->round_cell_duration = 100;
    this->round_cell_progress = 0;
    this->shape_cell_duration = 100;
    this->shape_cell_progress = 0;

    //calculates volume of the cell
    p=bp.begin();
    q=bp.begin();
    q++;
    float vol = 0.f;
    while (p != bp.end())
    {
        if(q==bp.end())
            q=bp.begin();

        float angle = (*q).ang-(*p).ang;
        if(angle<0.f)
            angle+=2.f*PI;

        vol+=(1.f/2.f) * (*p).dist * (*q).dist * sin(angle);
        p++;
        q++;
    }
    this->volume = vol;




}
void Cell::ProgressRotationShrink()
{
    roundCell();
    round_cell_progress++;
}

void Cell::ProgressRotationFinalizeShrink()
{
    std::list<PolarPair>::iterator p=bp.begin();
    std::list<PolarPair>::iterator q=rotate_bp.begin();
    while (p != bp.end())
    {
        p->ang=q->ang;
        p++;
        q++;
    }

    p=bp.begin();
    int count = 0;
    while(p!=bp.end())
    {
        count++;
        p++;
    }

    for(int i = 0; i<count; i++)
    {
        (*p).ang = 2.0f*PI*float(i)/float(count);
    }
    outerRadius = sqrt(volume/PI);

}

void Cell::ProgressRotationStretch()
{

    std::list<PolarPair>::iterator p=bp.begin();
    std::list<PolarPair>::const_iterator q=rotate_bp.begin();

    while(q!=rotate_bp.end())
    {

        if(p==bp.begin())
            cout << p->dist << " " << q->dist << endl;
        float tmp_dist = (*q).dist - (*p).dist;
        float help = tmp_dist/((float)shape_cell_duration-(float)shape_cell_progress);
        (*p).dist = (*p).dist + help;
        (*p).ang = (*q).ang;

        if(p==bp.begin())
            cout << p->dist << " " << q->dist << endl;

        //update the search for maximum radius
            if (p->dist > outerRadius) outerRadius=p->dist;

        p++;
        q++;
    }
    shape_cell_progress++;
}

void Cell::FinalizeRotation(float rotation)
{

    //update: rotate initial -- so that it shadows the current orientation
    std::list<PolarPair>::iterator p=initial_bp.begin();
    while (p != initial_bp.end())
    {
        p->ang = std::fmod(p->ang + rotation, 2.0f*PI);
        p++;
    }
}

