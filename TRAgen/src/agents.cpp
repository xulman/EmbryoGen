#include <GL/gl.h>
#include <cmath>
#include <list>
#include <vector>
#include <fstream>
#include <float.h>

#include "params.h"
#include "agents.h"
#include "rnd_generators.h"

#define DEG_TO_RAD(x) ((x)*M_PI/180.0f)

///whether to enable a time-lapse cell positions reporting
//#define REPORT_STATS

//link to the global params
extern ParamsClass params;

//link to the list of all agents
extern std::list<Cell*> agents;


//implementation of a function declared in params.h
///returns a-b in radians, function takes care of 2PI periodicity
float SignedAngularDifference(const float& a, const float& b)
{
	if (sin(a-b) > 0.)
		return acos(cos(a-b));
	else
		return -1. * acos(cos(a-b));

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
    this->stretch = false;
    this->desDir=0.f;

	 //we're in constructor, no dying yet
	 this->shouldDie=false;

	 //my new ID
	 this->ID=GetNewCellID();

	 //current cell velocity and others:
	 this->force=0.f;
	 this->force.type=forceNames::final;
	 this->listOfForces.reserve(50);
	 this->acceleration=0.f;
    this->velocity=0.f;
	 this->lastEasyForcesTime=params.currTime;

	 //social force:
	 //social force from all in the vicinity
    this->stretchCount=0;
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
	 this->delta_f=0.5f;

	 //body and sliding friction forces:
	 this->k=0.2f;
	 this->delta_o=0.5f;
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
		int flipCoin=floor(GetRandomUniform(0.f,4.99f));
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
		if (duration < 0) duration=1;
	}

	lastPhaseChange=params.currTime;
	nextPhaseChange=params.currTime+duration;
	phaseProgress=0.f;

	if (!G1start) initializeG2Phase(duration);
	//else G1 will be initialized explicitly from mother
}


Cell::Cell(const char* filename,
		const float R, const float G, const float B) :
			pos(0.f), orientation(0.f,1.f), bp(), former_bp(), 
			weight(1.f), initial_bp(), listOfForces(),
			isSelected(false), listOfFriends()
{
	colour.r=R;
	colour.g=G;
	colour.b=B;

	if (filename == NULL)
	{
		//no particular cell shape is prescribed,
		//going for default...
		bp.push_back(PolarPair(0.785,3.));
		bp.push_back(PolarPair(2.356,3.));
		bp.push_back(PolarPair(3.927,3.));
		bp.push_back(PolarPair(5.498,3.));
		outerRadius=3.;
	}
	else
	{
		std::ifstream file(filename);
		//TODO: add file read tests...
		float ignore;
		file >> pos.x >> pos.y >> ignore;
		file >> ignore;
		file >> orientation.ang >> orientation.dist;

		float ang,dist;
		while (file >> ang >> dist)
			  bp.push_back(PolarPair(ang,dist));

		file.close();

		//search for the maximum radius
		outerRadius=0.f;

		//subsample the list to keep min number of points
		#define MIN_BOUNDARY_POINTS		40

		//every keepFactor-th should be kept
		int keepFactor=bp.size()/MIN_BOUNDARY_POINTS -1;

		std::list<PolarPair>::iterator p=bp.begin();
		int erasedCnt=0;
		while (p != bp.end())
		{
			if (erasedCnt < keepFactor)
			{
				//erases current point and returns pointer to the next one
				p=bp.erase(p);
				erasedCnt++;
			}
			else
			{
				//keep this one
				erasedCnt=0;
				//update the search for maximum radius
				if (p->dist > outerRadius) outerRadius=p->dist;

				p++;
			}
		}

/*
		//now, re-adjust to cell diameter of 32um, hence radius=16um
		float stretch=17.f / outerRadius;
		p=bp.begin();
		while (p != bp.end())
		{
			p->dist *= stretch;
			++p;
		}
		outerRadius*=stretch;
		orientation.dist*=stretch;
*/

		//REPORT("bp list size is now " << bp.size());
		//this->ListBPs();
	}

	//back up the initial settings
    initial_bp=bp;
	initial_weight=weight;

	InitialSettings();
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
	orientation=buddy.orientation;
	//bp=buddy.bp;	//will fill mother of this cell
	outerRadius=buddy.outerRadius;

	weight=buddy.weight;
	initial_bp=buddy.initial_bp;
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


void Cell::calculateNewPosition(const Vector3d<float>& translation)
{
	//update: move
    //the stretching is done only if the cell is not reproducing itself
    if(curPhase!=G1Phase && curPhase!=Cytokinesis && curPhase!=Telophase)
    {
        //this part prepared the dreamShape
        if(!stretch)
        {

           // desiredDirection=GetRandomUniform(0.f,2*PI);
            float x1=cos(desiredDirection);
            float y1=sin(desiredDirection);

            float x2=x1+translation.x;
            float y2=y1+translation.y;

            desDir = atan2(y2,x2);
            //desiredSpeed=GetRandomUniform(0.0f, 0.8f);
            //new values for direction and speed are calculated
            //desDir = atan2(translation.y,translation.x);


            //dream shape is the shape of the original cell rotated by desired direction
            std::list<PolarPair>::iterator q=initial_bp.begin();
            dreamShape=initial_bp;
            std::list<PolarPair>::iterator p=dreamShape.begin();
            while (q != initial_bp.end())
            {
                float change = -initialOrientation+desDir;
                orientation.ang = desDir;


                p->ang = std::fmod(double(q->ang + change), 2.*PI);
                q++;
                p++;

            }

            //the cell is stretched
            CellStretch(desDir);
            dreamTransform=bp;
            stretch = true;

        }
        //if stretching is on progress, the bp of the cells and their interpolation during movement are calculted using the dreamShape and dreamTransform lists
        if(stretch)
        {
            stretchCount++;


            float smallestdist = 1000.f;
            int iter=0, finaliter=0;
            std::list<PolarPair>::iterator q = dreamShape.begin();
            std::list<PolarPair>::iterator r = dreamTransform.begin();
            while (r != dreamTransform.end())
            {
                float x1 = r->dist*cos(r->ang);
                float y1 = r->dist*sin(r->ang);
                float x2 = q->dist*cos(q->ang);
                float y2 = q->dist*sin(q->ang);

                float dist = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));

                if(dist<smallestdist)
                {
                    smallestdist=dist;
                    finaliter=iter;
                }
                iter++;
                r++;
            }

            std::list<PolarPair>::iterator p = bp.begin();
            q = dreamShape.begin();
            r = dreamTransform.begin();

            for(int i = 0; i<finaliter; i++)
            {
                r++;
            }

            while (p != bp.end())
            {
                if(r == dreamTransform.end())
                    r=dreamTransform.begin();

                float x1 = r->dist*cos(r->ang);
                float y1 = r->dist*sin(r->ang);
                float x2 = q->dist*cos(q->ang);
                float y2 = q->dist*sin(q->ang);

                float x3=x1+(stretchCount/5.f)*(x2-x1);
                float y3=y1+(stretchCount/5.f)*(y2-y1);

                p->ang=atan2(y3,x3);
                p->dist=x3/cos(p->ang);



                p++;
                q++;
                r++;
            }

            if(stretchCount==3)
            {
                stretchCount=0;
                stretch=false;

            }
        }
    }
    pos+=translation;
}


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
			glColor4f(0.5f,0.5f,0.5f, 0.8f);
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


void Cell::ListBPs(void) const
{
	if (!isSelected) return;

	std::cout << "\n   centre at [" << pos.x << "," << pos.y << "], ";
	std::cout << "orientation at " << orientation.ang << " (dist=" << orientation.dist << "), ";
	std::cout << "BPs polar coords (ang,dist):\n";

	std::list<PolarPair>::const_iterator p=bp.begin();
	int cnt=0;
	while (p != bp.end())
	{
		std::cout << "(" << p->ang << "," << p->dist << "), ";
		if (cnt % 5 == 4) std::cout << "\n";
		++p; ++cnt;
	}
	if (cnt % 5 != 0) std::cout << "\n";
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

	std::cout << "   centre at [" << pos.x << "," << pos.y << "], ";
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
						+(this->pos.y-buddy.pos.y)*(this->pos.y-buddy.pos.y) );
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

float Cell::CalcDistance2(const Cell& buddy,int& number,
	                       const float ext)
{
	//determine azimuth between me and my buddy
	const float angToBuddy=std::atan2(buddy.pos.y - pos.y, buddy.pos.x - pos.x);
	const float angFromBuddy=std::atan2(pos.y - buddy.pos.y, pos.x - buddy.pos.x);

	//_my and _buddy boundary points _nearest to the given angles,
	//nearest from both sides ("left" and "right")
	std::list<PolarPair>::const_iterator mnl,mnr,bnl,bnr;

	//FindNearest(angToBuddy,mn);
	//buddy.FindNearest(angFromBuddy,bn);

	//sweeping iterator
	std::list<PolarPair>::const_iterator p;

	float bdistl=-1000.f, bdistr=1000.f;
	p=bp.begin();
	while (p != bp.end())
	{
		float d=SignedAngularDifference(angToBuddy,p->ang);
		if ((d > 0) && (d < bdistr))
		{
			bdistr=d;
			mnr=p;
		}
		else
		if ((d < 0) && (d > bdistl))
		{
			bdistl=d;
			mnl=p;
		}

		++p;
	}

	bdistl=-1000.f; bdistr=1000.f;
	p=buddy.bp.begin();
	while (p != buddy.bp.end())
	{
		float d=SignedAngularDifference(angFromBuddy,p->ang);
		if ((d > 0) && (d < bdistr))
		{
			bdistr=d;
			bnr=p;
		}
		else
		if ((d < 0) && (d > bdistl))
		{
			bdistl=d;
			bnl=p;
		}

		++p;
	}

	if ((isSelected) && (ext < 0.01f))
		REPORT("signed ang.diff mnl-r=" << SignedAngularDifference(mnl->ang,mnr->ang)
		       << " rad, bnl-r=" << SignedAngularDifference(bnl->ang,bnr->ang)
				 << " rad");

	//now, the four iterators point at the boundary points nearest
	//(from both sides) to a connection between centres of the two cells
	//
	//check, what pair is the most parallel to the connection vector
	Vector3d<float> connection(buddy.pos.x - pos.x, buddy.pos.y - pos.y, 0.f);
	connection/=sqrt(connection.x*connection.x + connection.y*connection.y);

	//cartesian coordinates of the boundary points
	Vector3d<float> mBP,bBP,tmp;

	CellPolarToGlobalCartesian(*mnl,      pos, mBP, ext);
	CellPolarToGlobalCartesian(*bnr,buddy.pos, bBP, ext);
	tmp=bBP-mBP;
	tmp/=sqrt(tmp.x*tmp.x + tmp.y*tmp.y);
	bdistl=dotProduct(tmp,connection);

	CellPolarToGlobalCartesian(*mnr,pos, mBP, ext);
	tmp=bBP-mBP;
	tmp/=sqrt(tmp.x*tmp.x + tmp.y*tmp.y);
	bdistr=dotProduct(tmp,connection);

	//looking for acos(val) closer to either 0 or PI is to look for acos(val)
	//further from PI/2, which is to look for greater abs(val)
	if (fabsf(bdistr) > fabsf(bdistl)) mnl=mnr;

	//the best matching (starting) pair is the (mnl,bnr);
	//we're gonna use (mnr,bnl) as the "sweeping pair" (until we get angularly too far)
	mnr=mnl; bnl=bnr;
	
	//bdistl = will hold the minimum distance discovered so far (inc. penetration depths)
	//number (param of this function) = will hold the number of "penetrating" pairs
	bdistl=9999999.f; //safely way too big distance in microns
	number=0;

	//check the first direction...
	while (AngularDifference(angToBuddy,mnr->ang) < PI/3.f)
	{
		CellPolarToGlobalCartesian(*mnr,      pos, mBP, ext);
		CellPolarToGlobalCartesian(*bnl,buddy.pos, bBP, ext);
		tmp=bBP-mBP;
		bdistr=sqrt(tmp.x*tmp.x + tmp.y*tmp.y);

#ifdef DEBUG_CALCDISTANCE2
		//debug visualization
		if (isSelected)
		{
			pointsToShow.push_back(mBP);
			pointsToShow.push_back(bBP);
		}
#endif

		if (dotProduct(tmp,connection) < 0)
		{
			//penetration in this case
			++number;
			bdistr *= -1.f;

#ifdef DEBUG_CALCDISTANCE2
			//debug visualization
			if (isSelected)
			{
				pairsToShow.push_back(mBP);
				pairsToShow.push_back(bBP);
			}
#endif
		}

		//update the minimum distance discovered so far
		if (bdistr < bdistl) bdistl=bdistr;

		//move on the next pair
		if (mnr == bp.begin()) mnr=bp.end();
		mnr--;
		bnl++;
		if (bnl == buddy.bp.end()) bnl=buddy.bp.begin();
	}

	//get ready for the second direction
	mnr=mnl; bnl=bnr;

	//skip the already processed pair
	mnr++;
	if (mnr == bp.end()) mnr=bp.begin();
	if (bnl == buddy.bp.begin()) bnl=buddy.bp.end();
	bnl--;

	//check the second direction...
	while (AngularDifference(angToBuddy,mnr->ang) < PI/3.f)
	{
		CellPolarToGlobalCartesian(*mnr,      pos, mBP, ext);
		CellPolarToGlobalCartesian(*bnl,buddy.pos, bBP, ext);
		tmp=bBP-mBP;
		bdistr=sqrt(tmp.x*tmp.x + tmp.y*tmp.y);

#ifdef DEBUG_CALCDISTANCE2
		//debug visualization
		if (isSelected)
		{
			pointsToShow.push_back(mBP);
			pointsToShow.push_back(bBP);
		}
#endif

		if (dotProduct(tmp,connection) < 0)
		{
			//penetration in this case
			++number;
			bdistr *= -1.f;

#ifdef DEBUG_CALCDISTANCE2
			//debug visualization
			if (isSelected)
			{
				pairsToShow.push_back(mBP);
				pairsToShow.push_back(bBP);
			}
#endif
		}

		//update the minimum distance discovered so far
		if (bdistr < bdistl) bdistl=bdistr;

		//move on the next pair
		mnr++;
		if (mnr == bp.end()) mnr=bp.begin();
		if (bnl == buddy.bp.begin()) bnl=buddy.bp.end();
		bnl--;
	}

	if ((isSelected) && (ext < 0.01f))
		REPORT("dist=" << bdistl << " um (with ext=" << ext
					<< " um) to buddy at " << angToBuddy
		         << " rad, no. of colliding points is " << number);
	return(bdistl);
}


bool Cell::IsPointInCell(const Vector3d<float>& coord)
{
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
		 const float lweight = this->lambda + (1.-this->lambda)*( (1.+cos(Phi)) / 2.);

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
			CalcDistance2(*(fr->buddy),intersectPointsNo,this->delta_f);

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
    Vector3d<float> t(cos(ang - PI/2.),sin(ang - PI/2.),0.f);

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
	if ((fabsf(temp1x) > 4.f) || (fabsf(temp1y) > 4.f)) this->shouldDie=true;
	/* force is "relaxed"
	*/
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
		
        float RotationChange=GetRandomUniform(-1.f,1.f);
        //desiredDirection+=directionChangeLastAngle;
        desiredDirection=std::fmod(desiredDirection+RotationChange,2.f*PI);
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


	//what will the new cell shape look like?
	//at the moment, only rotates to match movement with shape
	ChangeDirection(timeDelta);
	//the above tells what will be desired velocity orientation after timeDelta

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

}


void Cell::calculateForces(const float timeDelta)
{
	//the forces for this run have already been initiated in the Cell::adjustShape()

	//just to get rid of silly warning about unused parameter
	float unused=timeDelta; unused=unused;

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
			int intersectPointsNo=0;
			//dist=CalcDistance(*buddy);
			dist=CalcDistance2(*buddy,intersectPointsNo);
			if (dist > 1000)
			{
				//TODO odstran az bude jistota, ze to funguje.. vypada, ze ale funguje
				std::cout << "PRUSER\n";
				//dist=CalcDistance2(*buddy,intersectPointsNo);
			}

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
	calculateAttractionForce(timeDelta);

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
		std::cout << "DEATH DUE TO EXTENSIVE STRESS\n";
		//colour.g=0.f;
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

    //std::cout << translation.x << " " << translation.y << std::endl;

	//backup before applying...
	former_bp=bp;
	former_pos=pos;

	//apply the translation
    calculateNewPosition(translation);
}


void Cell::Grow_Fill_Voids(void)
{
    int amount_small = bp.size();
    int amount_large = initial_bp.size();
    int missing = amount_large-amount_small;
    int distribution= trunc(missing/amount_small);
    int modulo = (amount_large%amount_small);

    std::list<PolarPair> help_bp;
    std::list<PolarPair>::iterator s=bp.begin();
    while (s != bp.end())
    {
        help_bp.push_back((*s));
        s++;
    }

    bp.clear();

    //distribute new points
    std::list<PolarPair>::iterator p=help_bp.begin();
    float prev_dist = (*p).dist;

    while (p != help_bp.end())
    {
        int loops;
        if(modulo != 0)
        {
            loops=distribution+1;
            modulo--;
        }
        else
        {
            loops=distribution;
        }

        std::list<PolarPair>::const_iterator r=p;
        r++;
        if(r==help_bp.end())
            r=help_bp.begin();
        bp.push_back(*p);
        for(int i=1; i<=loops; i++)
        {
            PolarPair pair(0.f,0.f);
            if(r!=help_bp.begin())
            {
                pair.ang = (*p).ang+(((*r).ang-(*p).ang)/(loops+1))*i;

                //the magic. just go step by step and draw it, you will understand
                float alpha = pair.ang-(*p).ang;
                float gamma = ((*r).ang-(*p).ang)/2.f-alpha;
                float delta;
                if(gamma<0.f)
                {
                    gamma*=-1.f;
                    delta = PI/2.0-gamma;

                }
                else
                {
                    delta = PI/2.0-gamma;
                    delta = PI-delta;
                }

                float beta= (((*r).ang-(*p).ang)/(loops+1))*i-(((*r).ang-(*p).ang)/(loops+1))*(i-1);
                float fi = PI-delta-beta;
                pair.dist = prev_dist*sin(fi)/sin(delta);
                prev_dist=pair.dist;


            }
            else
            {
                pair.ang = (*p).ang+(((*r).ang+2*PI-(*p).ang)/(loops+1))*i;
                float alpha = pair.ang-(*p).ang;
                if(alpha<0.f)
                {
                    alpha+=2*PI;
                }
                float gamma = ((*r).ang+2*PI-(*p).ang)/2.f-alpha;
                float delta;
                if(gamma<0.f)
                {
                    gamma*=-1.f;
                    delta = PI/2.0-gamma;

                }
                else
                {
                    delta = PI/2.0-gamma;
                    delta = PI-delta;
                }

                float beta= (((*r).ang+2*PI-(*p).ang)/(loops+1))*i-(((*r).ang+2*PI-(*p).ang)/(loops+1))*(i-1);
                float fi = PI-delta-beta;

                pair.dist = prev_dist*sin(fi)/sin(delta);
                prev_dist=pair.dist;

            }
            if(pair.ang>2*PI)
                pair.ang-=PI*2;

            bp.push_back(pair);


        }
        prev_dist=(*p).dist;
        p++;
    }

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
	//change back to yellow cells
	colour.r=1.f;
	colour.b=0.f;

	//re-set persistenceTime
   settleTime=GetRandomUniform(4.,10.);
   persistenceTime=GetRandomUniform(10.,20.);
	//direction change will occur soon (see initializeProphase())

	if (isSelected) REPORT("duration=" << duration << " minutes");

	bp.sort(SortPolarPair());

	grow_iterations=duration/params.incrTime;
	cur_grow_it=0;
	p_iter=0;
	Grow_Fill_Voids();
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
	//changes to blue cells (to denote mitosis)
	colour.r=0.f;
	colour.b=1.f;

	//shorten persistenceTime to stop cell movement fast
	persistenceTime=params.cellPhaseDuration[Prophase];
	desiredSpeed=0.f;

	//schedule next direction change nearly right after mitosis ends
	directionChangeCounter=0.f;
	directionChangeInterval=0.06f*params.cellCycleLength;

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

	//find boundary point at one cell pole
	FindNearest(orientation.ang,referenceBP);

	//we wish to elongate by some 35% in one direction
	referenceBP_base=referenceBP->dist;
	referenceBP_rate=0.35f * referenceBP_base;
	
	//now, _base+progress*_rate tells me where I should be
	//when the progress is some (0...1)
}

void Cell::initializeCytokinesis(const float duration)
{
	if (isSelected) REPORT("duration=" << duration << " minutes");

    std::list<PolarPair>::iterator lowerPoint;
	 FindMinorAxis(referenceBP, lowerPoint);

	//we wish to contract by some 85% in radius
	referenceBP_base=referenceBP->dist;
	referenceBP_rate=-0.85f * referenceBP_base;
	
	//now, _base+progress*_rate tells me where I should be
	//when the progress is some (0...1)
}


void Cell::progressG1Phase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
	Grow();
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

	//how much to elongate?
	float PeakElongation=referenceBP_rate*phaseProgress
				+ referenceBP_base - referenceBP->dist;
	if (isSelected) REPORT("PeakElongation=" << PeakElongation);

	if (PeakElongation < 0.f) return;

	//how wide? half of the boundary points should span 6*sigma
	//const float spread=float(MIN_BOUNDARY_POINTS/2) / 6.f;

	//slightly more than 180deg should span 6*sigma
	const float spread=DEG_TO_RAD(200.f) / 6.f;
	const float sigma=spread*spread;

	//okay, we need to elongate by PeakElongation at the reference pole
	std::list<PolarPair>::iterator p=bp.begin();
	while (p != bp.end())
	{
		float lAngle=AngularDifference(p->ang,orientation.ang);
		float rAngle=AngularDifference(p->ang,orientation.ang+PI);

		p->dist += PeakElongation*
			( expf(-0.5f*lAngle*lAngle/sigma) + expf(-0.5f*rAngle*rAngle/sigma) );

		//update the search for maximum radius
		if (p->dist > outerRadius) outerRadius=p->dist;

		++p;
	}
}

void Cell::progressCytokinesis(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);

	//how much contracted should we be by now?
	//note: _rate is negative now
	float ContractDist=referenceBP_base + referenceBP_rate*phaseProgress;
	float lprogress=1.f - ContractDist/referenceBP->dist;
	if (isSelected) REPORT("ContractDist=" << ContractDist << ", local progress=" << lprogress);

	if (ContractDist < 0.f) return;

	doCytokinesis(lprogress);
}


void Cell::finalizeG1Phase(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);
	bp=initial_bp;
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

	//we have not reached far, try to finish it
	if (phaseProgress < 0.98f)
	{
		phaseProgress=1.f;
		progressTelophase();
	}
}

void Cell::finalizeCytokinesis(void)
{
	if (isSelected) REPORT("phaseProgress=" << phaseProgress);

	//we have not reached far, try to finish it
	if (phaseProgress < 0.98f)
	{
		phaseProgress=1.f;
		progressCytokinesis();
	}

	//now tear the cells into two...

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

	 //I'm gonna split the list in the upperPoint and lowerPoint
	 //but before we need new cell...
	 Cell* cc=new Cell(*this, colour.r,colour.g,colour.b);
	 agents.push_back(cc);
	 //copy const. call InitialSettings(true) and so the new
	 //cell lacks initializeG1Phase() and init of bp list, and is already in G1Phase

	 //now, the split:

	 //move points from this cell to the second daughter
	 std::list<PolarPair>::iterator p=upperPoint;


	//obtain new geom. centre
	Vector3d<float> centre(0);
	int cnt=0;

	//sweeping BPs over the first daughter points
	//and moving at once
	while (p != lowerPoint)
	{
		//get Cartesian coordinates...
		Vector3d<float> tmp(p->dist*cos(p->ang), p->dist*sin(p->ang), 0.f);

		//...in a global coord system...
		tmp+=pos;

		//...and calc. geom. centre from these...
		centre+=tmp;
		cnt++;

		//...and move
		cc->bp.push_back(*p);
		p=bp.erase(p);

		//next BP
		//p++; -- erase() moves 'p' on the next one already
		if (p == bp.end()) p=bp.begin();
	}

	//finalize centre:
	centre.x/=(float)cnt;
	centre.y/=(float)cnt;

	//recalc the BPs for the new centre
	cc->outerRadius=0.f;
	p=cc->bp.begin();
	while (p != cc->bp.end())
	{
		Vector3d<float> tmp(p->dist*cos(p->ang), p->dist*sin(p->ang), 0.f);
		tmp+=pos;

		tmp.x-=centre.x;
		tmp.y-=centre.y;

		p->ang=atan2(tmp.y,tmp.x);
		p->dist=sqrt(tmp.x*tmp.x + tmp.y*tmp.y);

		if (p->dist > cc->outerRadius) cc->outerRadius=p->dist;

		p++;
	}
	cc->pos=centre;


	//obtain new geom. centre
	centre=0;
	cnt=0;

	//sweeping BPs over the second daughter points
	p=bp.begin();
	while (p != bp.end())
	{
		//get Cartesian coordinates...
		Vector3d<float> tmp(p->dist*cos(p->ang), p->dist*sin(p->ang), 0.f);

		//...in a global coord system...
		tmp+=pos;

		//...and calc. geom. centre from these
		centre+=tmp;
		cnt++;

		//next BP
		p++;
	}

	//finalize centre:
	centre.x/=(float)cnt;
	centre.y/=(float)cnt;

	//recalc the BPs for the new centre
	outerRadius=0.f;
	p=bp.begin();
	while (p != bp.end())
	{
		Vector3d<float> tmp(p->dist*cos(p->ang), p->dist*sin(p->ang), 0.f);
		tmp+=pos;

		tmp.x-=centre.x;
		tmp.y-=centre.y;

		p->ang=atan2(tmp.y,tmp.x);
		p->dist=sqrt(tmp.x*tmp.x + tmp.y*tmp.y);

		if (p->dist > outerRadius) outerRadius=p->dist;

		p++;
	}
	pos=centre;

	cc->initializeG1Phase(cc->nextPhaseChange - cc->lastPhaseChange);
	//this->initializeG1Phase(duration) will call my caller

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
		  q->dist *= (1.0f - progress/level);
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
		  q->dist *= (1.0f - progress/level);
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
		  q->dist *= (1.0f - progress/level);
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
		  q->dist *= (1.0f - progress/level);
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
		  angleDist = fabs(currentRelativeAngle - M_PI_2); 
		  if (angleDist < upperDist) 
		  {
				upperDist = angleDist;
				upperPoint = q;
		  }

		  // is the elevation of current point near to 3*PI/2?
		  // if so - store it as a candidate to lowerPoint
		  angleDist = fabs(currentRelativeAngle - 3*M_PI_2);
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
		  return (angle + 2*M_PI); 
	 
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

    /*
     * The loop slowly rounds the cell over the course of several frames. it tries to keep the volume of the cell at the same value
     */
    float value = sqrt(volume/PI);
    while(p!=bp.end())
    {

        float tmp_dist = value - (*p).dist;
        float help = tmp_dist/((float)round_cell_duration-(float)round_cell_progress);
        (*p).dist = (*p).dist + help;

            //update the search for maximum radius
            if (p->dist > outerRadius) outerRadius=p->dist;

        p++;

    }
}

void Cell::CellStretch(float DesDir2)
{

    //stretch by mail axis

    /*
     * this loop has a stretched dream shape established and the main idea is to go through all of the points and to move them towards their dreamShape position.
     * this is the basis of the movement, as the cell stretches out and then shrinks back later
     */



    std::list<PolarPair>::iterator p=dreamShape.begin();
    while(p!=dreamShape.end())
    {

        float help = p->ang-DesDir2;

        if(help<0)
            help*=-1;



        if(help < PI/2)
        {
            float a =sqrt(pow(p->dist,2)+pow(2*desiredSpeed,2)-2*p->dist*2*desiredSpeed*cos(PI-help));
            float gamma = asin((2*desiredSpeed/a)*sin(PI-help));
            p->dist=a;
            if(p->ang-DesDir2> 0)
                p->ang-=gamma;
            else
                p->ang+=gamma;
        }
        else
        {
            help -= PI*(-1);
            float a =sqrt(pow(p->dist,2)+pow(2*desiredSpeed,2)-2*p->dist*2*desiredSpeed*cos(PI-help));
            float gamma = asin((2*desiredSpeed/a)*sin(PI-help));
            p->dist=a;
            if(p->ang-DesDir2 > 0)
                p->ang-=gamma;
            else
                p->ang+=gamma;
        }
        p++;

    }

    //shrink by minor axis
    /*
     * As The cell stretches out in its major axis it needs to shrink in its minor axis. this calculation is not precise, and the actual volume of the cell flactuates because of it
     * It takes the fact that the cell doesnt stretch too much in the main axis into account
     */
    p=dreamShape.begin();
    while(p!=dreamShape.end())
    {
        float desDir = std::fmod(DesDir2+PI/2.f,2.f*PI);
        float help = p->ang-desDir;

        if(help<0)
            help*=-1;



        if(help < PI/2)
        {
            float a =sqrt(pow(p->dist,2)+pow(2*desiredSpeed,2)-2*p->dist*2*desiredSpeed*cos(help));
            float gamma = asin((2*desiredSpeed/a)*sin(help));
            p->dist=a;
            if(p->ang-desDir> 0)
                p->ang+=gamma;
            else
                p->ang-=gamma;
        }
        else
        {
            help -= PI*(-1);
            float a =sqrt(pow(p->dist,2)+pow(2*desiredSpeed,2)-2*p->dist*2*desiredSpeed*cos(help));
            float gamma = asin((2*desiredSpeed/a)*sin(help));
            p->dist=a;
            if(p->ang-desDir > 0)
                p->ang+=gamma;
            else
                p->ang-=gamma;
        }
        p++;

    }

}


float Cell::calculateVolume()
{
    std::list<PolarPair>::iterator p=bp.begin();
    std::list<PolarPair>::iterator q=bp.begin();
    q++;
    float vol = 0.f;
    /*
     * the loop goes through all points and calculates the size of small triangles specified by two neighbouring points and the center.
     * In the end it adds sizes of all triangles together.
     */
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
    return vol;
}
