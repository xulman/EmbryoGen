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
	//signal we want to have them displayed (even if cellsData is initially empty)
	private boolean          cellsShown = true;

	public
	void ToggleDisplayCells()
	{
		//add-or-remove from the scene
		for (Cell c : cellsData.values())
		for (Node n : c.sphereNodes)
			if (cellsShown) scene.removeChild(n);
			else            scene.addChild(n);

		//toggle the flag
		cellsShown ^= true;
	}

	/** only removes the cells from the scene, but keeps
	    displaying them (despite none exists afterwards) */
	public
	void RemoveCells()
	{
		//if cells are displayed, remove it first from the scene
		if (cellsShown)
		{
			for (Cell c : cellsData.values())
			for (Node n : c.sphereNodes)
				scene.removeChild(n);
		}
/*
		//also remove the vector Nodes from the scene, possibly
		if (vectorsShown)
		{
			//TODO
		}
*/
		cellsData.clear();
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
/*
		//also remove the vector Nodes from the scene, possibly
		if (vectorsShown)
		{
			//TODO
		}
*/
		cellsData.remove(c.ID);
	}

	/** this one injects shape object(s) into the cell */
	public
	void UpdateCellNodes(final Cell c)
	{
		//update the sphere Nodes (regardless of the actual value of the radius:
		//once Cell exists, all elements of its ...Nodes[] must not be null)
		for (int i=0; i < c.sphereRadii.length; ++i)
		{
			//negative radius is an agreed signal to remove the cell
			if (c.sphereRadii[i] < 0.0f)
			{
				//System.out.println("Removing ID "+c.ID);
				RemoveCell(c);
				return;
			}

			if (c.sphereNodes[i] == null)
			{
				//create and enable a new display element (Node)
				c.sphereNodes[i] = new Sphere(1.0f, 12);
				if (cellsShown) scene.addChild(c.sphereNodes[i]);
			}

			//update...
			c.sphereNodes[i].setScale(new GLVector(c.sphereRadii[i],3));
			c.sphereNodes[i].setPosition(new GLVector(c.sphereCentres[3*i+0],
			                                          c.sphereCentres[3*i+1],
			                                          c.sphereCentres[3*i+2]));
			c.sphereNodes[i].setMaterial(materials[c.sphereColors[i] % materials.length]);
		}

		//update the vector Nodes
		//TODO
	}

/*
		final Sphere sph1 = new Sphere(1.0f, 12);
		sph1.setMaterial( material );
		sph1.setPosition( new GLVector(0.0f, 0.0f, 0.0f) );
		getScene().addChild(sph1);
		//System.out.println("Sphere is made of "+sph1.getVertices().capacity()+" vertices");
		//getScene().removeChild(sph1);
		final Cylinder cyl1 = new Cylinder(0.1f,2.0f,4);

		cyl1.setMaterial(material);
		sph1.addChild(cyl1);

		//here, the order of getRotation() and setPosition() does not matter
		//(seems that it rotates always around object's own center, rotates the model)
		//
		//if object is child to another object, the setPosition() is relative to the
		//parent object's position

		//this is the current orientation of the cylinder's main axis
		final GLVector mainAxis1 = new GLVector(0.0f,1.0f,0.0f); //NB: already normalized

		//this is the vector that we aim to display as a narrow cylinder
		GLVector vec1 = new GLVector(2.0f,0.0f,0.0f);
		ReOrientNode(cyl1,mainAxis1,vec1);
		System.out.println("main axis=("+mainAxis1.x()+","+mainAxis1.y()+","+mainAxis1.z()+")");
*/
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

		//extra case when the two orientations are co-linear
		if (Math.abs(rotAngle) < 0.01f || Math.abs(rotAngle-3.14159f) < 0.01f)
		{
			//System.out.println("rotating with supporting vector");
			tmpVec = new GLVector(1.0f,1.0f,0.0f);
			//NB: tmpVec still might be co-linear with the currentNormalizedOrientVec...
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
}
