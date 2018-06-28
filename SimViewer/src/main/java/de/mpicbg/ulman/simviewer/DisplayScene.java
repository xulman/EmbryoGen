package de.mpicbg.ulman.simviewer;

import cleargl.GLVector;
import graphics.scenery.*;
import graphics.scenery.backends.Renderer;
import graphics.scenery.Material.CullingMode;

import java.util.Map;
import java.util.HashMap;
import de.mpicbg.ulman.simviewer.agents.Cell;

/**
 * Adapted from TexturedCubeJavaExample.java from the scenery project,
 * originally created by kharrington on 7/6/16.
 *
 * Current version is created by Vladimir Ulman, 2018.
 */
public class DisplayScene extends SceneryBase implements Runnable
{
	/** constructor to create an empty window */
	public
	DisplayScene(final float[] sOffset, final float[] sSize)
	{
		//the last param determines whether REPL is wanted, or not
		super("EmbryoGen - live view using scenery", 1024,512, false);

		if (sOffset.length != 3 || sSize.length != 3)
			throw new RuntimeException("Offset and Size must be 3-items long: 3D world!");

		//init the scene dimensions
		sceneOffset = sOffset.clone();
		sceneSize   = sSize.clone();

		//init the colors -- the material lookup table
		materials = new Material[7];
		(materials[0] = new Material()).setDiffuse( new GLVector(1.0f, 1.0f, 1.0f) );
		(materials[1] = new Material()).setDiffuse( new GLVector(1.0f, 0.0f, 0.0f) );
		(materials[2] = new Material()).setDiffuse( new GLVector(0.0f, 1.0f, 0.0f) );
		(materials[3] = new Material()).setDiffuse( new GLVector(0.0f, 0.0f, 1.0f) );
		(materials[4] = new Material()).setDiffuse( new GLVector(0.0f, 1.0f, 1.0f) );
		(materials[5] = new Material()).setDiffuse( new GLVector(1.0f, 0.0f, 1.0f) );
		(materials[6] = new Material()).setDiffuse( new GLVector(1.0f, 1.0f, 0.0f) );
		for (Material m : materials)
		{
			m.setCullingMode(CullingMode.None);
			m.setAmbient(  new GLVector(1.0f, 1.0f, 1.0f) );
			m.setSpecular( new GLVector(1.0f, 1.0f, 1.0f) );
		}
	}

	/** 3D position of the scene, to position well the lights and camera */
	final float[] sceneOffset;
	/** 3D size (diagonal vector) of the scene, to position well the lights and camera */
	final float[] sceneSize;

	/** fixed lookup table with colors, in the form of materials... */
	final Material[] materials;

	/** short cut to the root Node underwhich all displayed objects should be hooked up */
	Node scene;

	/** reference for the camera to be able to enable/disable head lights */
	Camera cam;
	//----------------------------------------------------------------------------


	/** initializes the scene with one camera and multiple lights */
	public
	void init()
	{
		setRenderer( Renderer.createRenderer(getHub(), getApplicationName(), getScene(), getWindowWidth(), getWindowHeight()));
		getHub().add(SceneryElement.Renderer, getRenderer());

		//the overall down scaling of the displayed objects such that moving around
		//the scene with Scenery is vivid (move step size is fixed in Scenery), at
		//the same time we want the objects and distances to be defined with our
		//non-scaled coordinates -- so we hook everything underneath the fake object
		//that is downscaled (and consequently all is downscaled too) but defined with
		//at original scale (with original coordinates and distances)
		final float DsFactor = 0.01f;

		//introduce an invisible "fake" object
		scene = new Box(new GLVector(0.0f,3));
		scene.setScale(new GLVector(DsFactor,3));
		getScene().addChild(scene);

		//camera position, looking from the front into the scene centre
		//NB: z-position should be such that the FOV covers the whole scene
		float xCam = (sceneOffset[0] + 0.5f*sceneSize[0]) *DsFactor;
		float yCam = (sceneOffset[1] + 0.5f*sceneSize[1]) *DsFactor;
		float zCam = (sceneOffset[2] + 1.7f*sceneSize[2]) *DsFactor;
		cam = new DetachedHeadCamera();
		cam.setPosition( new GLVector(xCam,yCam,zCam) );
		cam.perspectiveCamera(50.0f, getRenderer().getWindow().getWidth(), getRenderer().getWindow().getHeight(), 0.05f, 1000.0f);
		cam.setActive( true );
		getScene().addChild(cam);

		//a cam-attached light mini-ramp
		//------------------------------
		xCam =  0.3f*sceneSize[0] *DsFactor;
		yCam =  0.2f*sceneSize[1] *DsFactor;
		zCam = -0.1f*sceneSize[2] *DsFactor;

		float radius = 2.5f*sceneSize[1] *DsFactor;

		headLights = new PointLight[6];
		(headLights[0] = new PointLight(radius)).setPosition(new GLVector(-xCam,-yCam,zCam));
		(headLights[1] = new PointLight(radius)).setPosition(new GLVector( 0.0f,-yCam,zCam));
		(headLights[2] = new PointLight(radius)).setPosition(new GLVector(+xCam,-yCam,zCam));
		(headLights[3] = new PointLight(radius)).setPosition(new GLVector(-xCam,+yCam,zCam));
		(headLights[4] = new PointLight(radius)).setPosition(new GLVector( 0.0f,+yCam,zCam));
		(headLights[5] = new PointLight(radius)).setPosition(new GLVector(+xCam,+yCam,zCam));

		//two light ramps at fixed positions
		//----------------------------------
		//elements of coordinates of positions of lights
		final float xLeft   = (sceneOffset[0] + 0.05f*sceneSize[0]) *DsFactor;
		final float xCentre = (sceneOffset[0] + 0.50f*sceneSize[0]) *DsFactor;
		final float xRight  = (sceneOffset[0] + 0.95f*sceneSize[0]) *DsFactor;

		final float yTop    = (sceneOffset[1] + 0.05f*sceneSize[1]) *DsFactor;
		final float yBottom = (sceneOffset[1] + 0.95f*sceneSize[1]) *DsFactor;

		final float zNear = (sceneOffset[2] + 1.3f*sceneSize[2]) *DsFactor;
		final float zFar  = (sceneOffset[2] - 0.3f*sceneSize[2]) *DsFactor;

		//tuned such that, given current light intensity and fading, the rear cells are dark yet visible
		radius = 1.8f*sceneSize[1] *DsFactor;

		//create the lights, one for each upper corner of the scene
		fixedLights = new PointLight[2][6];
		(fixedLights[0][0] = new PointLight(radius)).setPosition(new GLVector(xLeft  ,yTop   ,zNear));
		(fixedLights[0][1] = new PointLight(radius)).setPosition(new GLVector(xLeft  ,yBottom,zNear));
		(fixedLights[0][2] = new PointLight(radius)).setPosition(new GLVector(xCentre,yTop   ,zNear));
		(fixedLights[0][3] = new PointLight(radius)).setPosition(new GLVector(xCentre,yBottom,zNear));
		(fixedLights[0][4] = new PointLight(radius)).setPosition(new GLVector(xRight ,yTop   ,zNear));
		(fixedLights[0][5] = new PointLight(radius)).setPosition(new GLVector(xRight ,yBottom,zNear));

		(fixedLights[1][0] = new PointLight(radius)).setPosition(new GLVector(xLeft  ,yTop   ,zFar));
		(fixedLights[1][1] = new PointLight(radius)).setPosition(new GLVector(xLeft  ,yBottom,zFar));
		(fixedLights[1][2] = new PointLight(radius)).setPosition(new GLVector(xCentre,yTop   ,zFar));
		(fixedLights[1][3] = new PointLight(radius)).setPosition(new GLVector(xCentre,yBottom,zFar));
		(fixedLights[1][4] = new PointLight(radius)).setPosition(new GLVector(xRight ,yTop   ,zFar));
		(fixedLights[1][5] = new PointLight(radius)).setPosition(new GLVector(xRight ,yBottom,zFar));

		//common settings of all lights, front fixed ramp is currently in use
		//------------------------------
		final GLVector lightsColor = new GLVector(1.0f, 1.0f, 1.0f);
		for (PointLight l : headLights)
		{
			l.setIntensity(20.0f);
			l.setEmissionColor(lightsColor);
		}
		for (PointLight l : fixedLights[0])
		{
			l.setIntensity(10.0f);
			l.setEmissionColor(lightsColor);
		}
		for (PointLight l : fixedLights[1])
		{
			l.setIntensity(10.0f);
			l.setEmissionColor(lightsColor);
		}

		//enable the fixed ramp lights
		ToggleFixedLights();
	}

	/** runs the scenery rendering backend in a separate thread */
	public
	void run()
	{
		//this is the main loop: the code "blocks" here until the main window is closed,
		//and that's gonna be the job of this thread
		this.main();
	}

	/** attempts to close this rendering window */
	public
	void stop()
	{
		this.close();
	}
	//----------------------------------------------------------------------------


	//initiated in this.init() to match their state flags below
	private PointLight[][] fixedLights;
	private PointLight[]   headLights;

	public enum fixedLightsState { FRONT, REAR, BOTH, NONE };

	//the state flags of the lights
	private fixedLightsState fixedLightsChoosen = fixedLightsState.NONE;
	private boolean          headLightsChoosen  = false;

	public
	fixedLightsState ToggleFixedLights()
	{
		final Node camRoot = cam.getParent();

		switch (fixedLightsChoosen)
		{
		case FRONT:
			for (PointLight l : fixedLights[0]) camRoot.removeChild(l);
			for (PointLight l : fixedLights[1]) camRoot.addChild(l);
			fixedLightsChoosen = fixedLightsState.REAR;
			break;
		case REAR:
			for (PointLight l : fixedLights[0]) camRoot.addChild(l);
			fixedLightsChoosen = fixedLightsState.BOTH;
			break;
		case BOTH:
			for (PointLight l : fixedLights[0]) camRoot.removeChild(l);
			for (PointLight l : fixedLights[1]) camRoot.removeChild(l);
			fixedLightsChoosen = fixedLightsState.NONE;
			break;
		case NONE:
			for (PointLight l : fixedLights[0]) camRoot.addChild(l);
			fixedLightsChoosen = fixedLightsState.FRONT;
			break;
		}

		//report the current state
		return fixedLightsChoosen;
	}

	public
	boolean ToggleHeadLights()
	{
		headLightsChoosen ^= true; //toggles the state

		if (headLightsChoosen)
			for (PointLight l : headLights) cam.addChild(l);
		else
			for (PointLight l : headLights) cam.removeChild(l);

		return headLightsChoosen;
	}
	//----------------------------------------------------------------------------


	private Cylinder[] axesData = null;
	private boolean    axesShown = false;

	public
	void ToggleDisplayAxes()
	{
		//first run, init the data
		if (axesData == null)
		{
			axesData = new Cylinder[] {
				new Cylinder(1.0f,30.f,4),
				new Cylinder(1.0f,30.f,4),
				new Cylinder(1.0f,30.f,4)};

			//set material - color
			//NB: RGB colors ~ XYZ axes
			axesData[0].setMaterial(materials[1]);
			axesData[1].setMaterial(materials[2]);
			axesData[2].setMaterial(materials[3]);

			//set orientation for x,z axes
			ReOrientNode(axesData[0],new GLVector(0.0f,1.0f,0.0f),new GLVector(1.0f,0.0f,0.0f));
			ReOrientNode(axesData[2],new GLVector(0.0f,1.0f,0.0f),new GLVector(0.0f,0.0f,1.0f));

			//place all axes into the scene centre
			final GLVector centre = new GLVector(
				(sceneOffset[0] + 0.5f*sceneSize[0]),
				(sceneOffset[1] + 0.5f*sceneSize[1]),
				(sceneOffset[2] + 0.5f*sceneSize[2]));
			axesData[0].setPosition(centre);
			axesData[1].setPosition(centre);
			axesData[2].setPosition(centre);
		}

		//add-or-remove from the scene
		for (Node n : axesData)
			if (axesShown) scene.removeChild(n);
			else           scene.addChild(n);

		//toggle the flag
		axesShown ^= true;
	}
	//----------------------------------------------------------------------------


	private Line[]  borderData = null;
	private boolean borderShown = false;

	public
	void ToggleDisplaySceneBorder()
	{
		//first run, init the data
		if (borderData == null)
		{
			borderData = new Line[] { new Line(6), new Line(6),
			                          new Line(6), new Line(6)};

			final GLVector sxsysz = new GLVector(sceneOffset[0]             , sceneOffset[1]             , sceneOffset[2]             );
			final GLVector lxsysz = new GLVector(sceneOffset[0]+sceneSize[0], sceneOffset[1]             , sceneOffset[2]             );
			final GLVector sxlysz = new GLVector(sceneOffset[0]             , sceneOffset[1]+sceneSize[1], sceneOffset[2]             );
			final GLVector lxlysz = new GLVector(sceneOffset[0]+sceneSize[0], sceneOffset[1]+sceneSize[1], sceneOffset[2]             );
			final GLVector sxsylz = new GLVector(sceneOffset[0]             , sceneOffset[1]             , sceneOffset[2]+sceneSize[2]);
			final GLVector lxsylz = new GLVector(sceneOffset[0]+sceneSize[0], sceneOffset[1]             , sceneOffset[2]+sceneSize[2]);
			final GLVector sxlylz = new GLVector(sceneOffset[0]             , sceneOffset[1]+sceneSize[1], sceneOffset[2]+sceneSize[2]);
			final GLVector lxlylz = new GLVector(sceneOffset[0]+sceneSize[0], sceneOffset[1]+sceneSize[1], sceneOffset[2]+sceneSize[2]);

			//C-shape around the front face (one edge missing)
			borderData[0].addPoint(lxlysz);
			borderData[0].addPoint(sxlysz);
			borderData[0].addPoint(sxsysz);
			borderData[0].addPoint(lxsysz);
			borderData[0].setMaterial(materials[1]);

			//the same around the right face
			borderData[1].addPoint(lxlylz);
			borderData[1].addPoint(lxlysz);
			borderData[1].addPoint(lxsysz);
			borderData[1].addPoint(lxsylz);
			borderData[1].setMaterial(materials[3]);

			//the same around the rear face
			borderData[2].addPoint(sxlylz);
			borderData[2].addPoint(lxlylz);
			borderData[2].addPoint(lxsylz);
			borderData[2].addPoint(sxsylz);
			borderData[2].setMaterial(materials[1]);

			//the same around the left face
			borderData[3].addPoint(sxlysz);
			borderData[3].addPoint(sxlylz);
			borderData[3].addPoint(sxsylz);
			borderData[3].addPoint(sxsysz);
			borderData[3].setMaterial(materials[3]);

			for (Line l : borderData)
			{
				l.addPoint(sxsysz);
				l.addPoint(sxsysz);
				l.setEdgeWidth(0.02f);
			}
		}

		//add-or-remove from the scene
		for (Node n : borderData)
			if (borderShown) scene.removeChild(n);
			else             scene.addChild(n);

		//toggle the flag
		borderShown ^= true;
	}
	//----------------------------------------------------------------------------


	/** these guys will be displayed on the display */
	public Map<Integer,Cell> cellsData  = new HashMap<Integer,Cell>();

	/** signals we want to have cells (spheres) displayed (even if cellsData is initially empty) */
	private boolean cellsShown = true;

	/** signals we want to have cell forces (vectors) displayed */
	private boolean vectorsShown = false;

	/** cell forces are typically small in magnitude compared to the cell size,
	    this defines the current magnification applied when displaying the force vectors */
	private float vectorsStretch = 1000.f;

	public
	void ToggleDisplayCells()
	{
		//add-or-remove from the scene
		if (cellsShown)
			cellsData.values().forEach( c ->
				{ for (final Node n : c.sphereNodes) if (n != null) scene.removeChild(n); } );
			//NB: Cell might not have spheres defined yet (while vectors exist which justifies the existence of this object),
			//    hence we better test for their existence
		else
			cellsData.values().forEach( c ->
				{ for (final Node n : c.sphereNodes) if (n != null) scene.addChild(n); } );

		//toggle the flag
		cellsShown ^= true;
	}

	public
	void ToggleDisplayVectors()
	{
		//add-or-remove from the scene
		if (vectorsShown)
		{
			cellsData.values().forEach( c ->
				{ for (final Node n : c.forceNodes) if (n != null) scene.removeChild(n); } );
			//NB: Cell might not have vector defined yet (while spheres exist which justifies the existence of this object),
			//    hence we better test for their existence

			for (Material m : materials)
				m.setCullingMode(CullingMode.None);
		}
		else
		{
			cellsData.values().forEach( c ->
				{ for (final Node n : c.forceNodes) if (n != null) scene.addChild(n); } );

			for (Material m : materials)
				m.setCullingMode(CullingMode.Front);
		}

		//toggle the flag
		vectorsShown ^= true;
	}

	float getVectorsStretch()
	{ return vectorsStretch; }

	void setVectorsStretch(final float vs)
	{
		//update the stretch factor...
		vectorsStretch = vs;

		//...and rescale all vectors presently existing in the system
		final GLVector scaleVec = new GLVector(vectorsStretch,3);
		cellsData.values().forEach( c ->
			{ for (final Node n : c.forceNodes) if (n != null) n.setScale(scaleVec); } );
	}


	/** only removes the cells from the scene, but keeps
	    displaying them (despite none exists afterwards) */
	public
	void RemoveCells()
	{
		for (Cell c : cellsData.values().toArray(new Cell[0])) RemoveCell(c);

		//NB: just to make sure... should be empty by now
		if (cellsData.isEmpty() == false)
			throw new RuntimeException("RemoveCells() failed to empty the cell array.");
	}

	public
	void RemoveCell(final int ID)
	{
		RemoveCell(cellsData.get(ID));
	}

	public
	void RemoveCell(final Cell c)
	{
		if (c == null) return;

		//if cells are displayed, remove it first from the scene
		if (cellsShown)
		{
			for (Node n : c.sphereNodes)
			if (n != null)
				scene.removeChild(n);
		}

		//also remove the vector Nodes from the scene, possibly
		if (vectorsShown)
		{
			for (Node n : c.forceNodes)
			if (n != null)
				scene.removeChild(n);
		}

		cellsData.remove(c.ID);
	}

	/** this one updates, or injects new, cell shape object(s) into the scene */
	public
	void UpdateCellSphereNodes(final Cell c)
	{
		//update the sphere Nodes (regardless of the actual value of the radius:
		//once Cell exists, all elements of its ...Nodes[] must not be null
		for (int i=0; i < c.sphereRadii.length; ++i)
		{
			//negative radius is an agreed signal to remove the cell
			if (c.sphereRadii[i] < 0.0f)
			{
				//System.out.println("Removing ID "+c.ID);
				RemoveCell(c);
				return;
			}

// ------------ DEBUGING ------------
try {
			if (c.sphereNodes[i] == null)
			{
				//create and enable a new display element (Node)
				c.sphereNodes[i] = new Sphere(1.0f, 12);
				if (cellsShown) scene.addChild(c.sphereNodes[i]);
			}
}
catch ( NullPointerException e )
{
	//this comes up from time to time, why?
	//from the complains it seems that either scene == null,
	//or c.sphereNodes[i] == null

	//hence, a bit of debug...
	System.out.println("Cell: c="+c);
	System.out.println("i="+i+", radii.length="+c.sphereRadii.length);
	for (int qq=0; qq < c.sphereNodes.length; ++qq)
		System.out.println("nodes["+qq+"]="+c.sphereNodes[qq]);
	System.out.println("scene="+scene);

	//pass it further down.... (to stop)
	throw e;
}
// ------------ DEBUGING ------------

			//update...
			c.sphereNodes[i].setScale(new GLVector(c.sphereRadii[i],3));
			c.sphereNodes[i].setPosition(new GLVector(c.sphereCentres[3*i+0],
			                                          c.sphereCentres[3*i+1],
			                                          c.sphereCentres[3*i+2]));
			c.sphereNodes[i].setMaterial(materials[c.sphereColors[i] % materials.length]);
		}
	}

	/** this one updates, or injects new, cell force vectors' object(s) into the scene */
	public
	void UpdateCellVectorNodes(final Cell c)
	{
		//update the vector Nodes
		//once Cell exists, all elements of its ...Nodes[] must not be null
		for (int i=0; i < c.forceColors.length; ++i)
		{
			//remove the old vector from the scene (if displayed and there was one)
			if (vectorsShown && c.forceNodes[i] != null)
				scene.removeChild(c.forceNodes[i]);

			//create and enable (down below) a new display element (Node)
			//NB: for Line elements, one could consider clearPoints() and updating them,
			//    but that's cumbersome (matching IDs, removing, updating...)
			c.forceNodes[i] = CreateVector(new GLVector(c.forceVectors[3*i+0],
			                                            c.forceVectors[3*i+1],
			                                            c.forceVectors[3*i+2]));
			//update (position and scale)...
			c.forceNodes[i].setPosition(new GLVector(c.forceBases[3*i+0],
			                                         c.forceBases[3*i+1],
			                                         c.forceBases[3*i+2]));
			c.forceNodes[i].setScale(new GLVector(vectorsStretch,3));

			//update (material -- color)...
			final Material m = materials[c.forceColors[i] % materials.length];
			c.forceNodes[i].runRecursive(nn -> nn.setMaterial(m));

			if (vectorsShown) scene.addChild(c.forceNodes[i]);
		}
	}
	//----------------------------------------------------------------------------


	/**
	Rotates the node such that its orientation (whatever it is for the node, e.g.
	the axis of rotational symmetry in a cylinder) given with _normalized_
	currentNormalizedOrientVec will match the new orientation newOrientVec.
	The normalized variant of newOrientVec will be stored into the currentNormalizedOrientVec.
	*/
	public
	void ReOrientNode(final Node node, final GLVector currentNormalizedOrientVec,
	                  final GLVector newOrientVec)
	{
		//plan: vector/cross product of the initial object's orientation and the new orientation,
		//and rotate by angle that is taken from the scalar product of the two

		//the rotate angle
		final float rotAngle = (float)Math.acos(currentNormalizedOrientVec.times(newOrientVec.getNormalized()));

		//for now, the second vector for the cross product
		GLVector tmpVec = newOrientVec;

		//two special cases when the two orientations are (nearly) colinear:
		//
		//a) the same direction -> nothing to do (don't even update the currentNormalizedOrientVec)
		if (Math.abs(rotAngle) < 0.01f) return;
		//
		//b) the opposite direction -> need to "flip"
		if (Math.abs(rotAngle-Math.PI) < 0.01f)
		{
			//define non-colinear helping vector, e.g. take a perpendicular one
			tmpVec = new GLVector(-newOrientVec.y(), newOrientVec.x(), 0.0f);
		}

		//axis along which to perform the rotation
		tmpVec = currentNormalizedOrientVec.cross(tmpVec).normalize();
		node.getRotation().rotateByAngleNormalAxis(rotAngle, tmpVec.x(),tmpVec.y(),tmpVec.z());

		//System.out.println("rot axis=("+tmpVec.x()+","+tmpVec.y()+","+tmpVec.z()
		//                   +"), rot angle="+rotAngle+" rad");

		//update the current orientation
		currentNormalizedOrientVec.minusAssign(currentNormalizedOrientVec);
		currentNormalizedOrientVec.plusAssign(newOrientVec);
		currentNormalizedOrientVec.normalize();
	}


	/** Creates a vector node, that needs to be setMaterial'ed(), setPosition'ed(), and
	    addChild'ed(), as a line with two perpendicular triangles as a "3D arrow".
	    The base of the arrow head is a cross. */
	Line CreateVector(final GLVector v)
	{
		final Line l = new Line(10);
		l.setEdgeWidth(0.02f);

		/*
		  /|\
		 / | \
		/--+--\
		   |
		   |
		   |
		Vector is created as the main segment (drawn vertically bottom to up),
		then 3 segments to build a triangle and then another triangle perpendicular
		to the former one; altogehter 7 segments drawn sequentially
		*/

		//the main "vertical" segment of the vector
		l.addPoint(new GLVector(0.f,3));
		l.addPoint(v);

		//the first triangle:
		//the shape of the triangle
		final float V = 0.1f * v.magnitude();

		//vector base is perpendicular to the input vector v
		GLVector base = new GLVector(-v.y(), v.x(), 0.0f);
		base.timesAssign(new GLVector(V/base.magnitude(),3));

		l.addPoint(v.times(0.8f).plus(base));
		l.addPoint(v.times(0.8f).minus(base));
		l.addPoint(v);

		//the second triangle:
		base = base.cross(v);
		base.timesAssign(new GLVector(V/base.magnitude(),3));

		l.addPoint(v.times(0.8f).plus(base));
		l.addPoint(v.times(0.8f).minus(base));
		l.addPoint(v);

		//add two mandatory fake points that are never displayed
		l.addPoint(v);
		l.addPoint(v);

		return l;
	}

	/** Creates a vector node, that needs to be setMaterial'ed(), setPosition'ed(), and
	    addChild'ed(), as a line with a pyramid as a "3D arrow".
	    The base of the arrow head is a square with diagonals. */
	Line CreateVector_Pyramid(final GLVector v)
	{
		final Line l = new Line(14);
		l.setEdgeWidth(0.02f);

		/*
		  /|\
		 / | \
		/--+--\
		   |
		   |
		   |
		Vector is created as the main segment (drawn vertically bottom to up),
		then 3 segments to build a triangle and then another triangle perpendicular
		to the former one; altogehter 7 segments drawn sequentially
		*/

		//the main "vertical" segment of the vector
		l.addPoint(new GLVector(0.f,3));
		l.addPoint(v);

		//the first triangle:
		//the shape of the triangle
		final float V = 0.1f * v.magnitude();

		//vector base is perpendicular to the input vector v
		GLVector base = new GLVector(-v.y(), v.x(), 0.0f);
		base.timesAssign(new GLVector(V/base.magnitude(),3));

		GLVector a,b,c,d;
		a = v.times(0.8f).plus(base);
		c = v.times(0.8f).minus(base);
		l.addPoint(a);
		l.addPoint(c);
		l.addPoint(v);

		//the second triangle:
		base = base.cross(v);
		base.timesAssign(new GLVector(V/base.magnitude(),3));

		b = v.times(0.8f).plus(base);
		d = v.times(0.8f).minus(base);
		l.addPoint(b);
		l.addPoint(d);

		//the base
		l.addPoint(a);
		l.addPoint(b);
		l.addPoint(c);
		l.addPoint(d);

		//finish the 2nd triangle
		l.addPoint(v);

		//add two mandatory fake points that are never displayed
		l.addPoint(v);
		l.addPoint(v);

		return l;
	}

	/** Creates a vector node, that needs to be setMaterial'ed(), setPosition'ed(), and
	    addChild'ed(), as a line with a fancy cone as a "3D arrow".
	    The Scenery.Cone is used for the cone and is nested under the returned node.
	    The base of the arrow head is a circle.

	    Since the vector consists of two different graphics elements, we better
	    return common ancester type of them because any method of the Node can be
	    applied on the subsequent objects too. */
	Node CreateVector_Cone(final GLVector v)
	{
		final Line l = new Line(4);
		l.setEdgeWidth(0.02f);

		/* .
		  / \
		 /   \
		/-----\
		   |
		   |
		   |
		Vector is created as the main segment (drawn vertically bottom to up)
		with a cone sitting on top of it
		*/

		//the main "vertical" segment of the vector
		l.addPoint(new GLVector(0.f,3));
		l.addPoint(v.times(0.95f));
		l.addPoint(v);
		l.addPoint(v);

		//the cone
		//the shape of the cone
		final float V = 0.1f * v.magnitude();
		final Cone c = new Cone(V,2.0f*V,6);
		ReOrientNode(c,new GLVector(0.0f,1.0f,0.0f),v);
		c.setPosition(v.times(0.8f));
		l.addChild(c);

		return l;
	}
}
