package de.mpicbg.ulman.simviewer;

import cleargl.GLVector;
import graphics.scenery.*;
import graphics.scenery.backends.Renderer;
import graphics.scenery.Material.CullingMode;
import graphics.scenery.controls.InputHandler;
import org.scijava.ui.behaviour.ClickBehaviour;

import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;

import de.mpicbg.ulman.simviewer.aux.Point;
import de.mpicbg.ulman.simviewer.aux.Line;
import de.mpicbg.ulman.simviewer.aux.Vector;

/**
 * Adapted from TexturedCubeJavaExample.java from the scenery project,
 * originally created by kharrington on 7/6/16.
 *
 * This file was created and is being developed by Vladimir Ulman, 2018.
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
		(materials[3] = new Material()).setDiffuse( new GLVector(0.2f, 0.4f, 1.0f) ); //lighter blue
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
	Node scene = null;

	/** reference for the camera to be able to enable/disable head lights */
	Camera cam;
	//----------------------------------------------------------------------------


	/** initializes the scene with one camera and multiple lights */
	public
	void init()
	{
		final Renderer r = Renderer.createRenderer(getHub(), getApplicationName(), getScene(), getWindowWidth(), getWindowHeight());
		setRenderer(r);
		getHub().add(SceneryElement.Renderer, getRenderer());

		//the overall down scaling of the displayed objects such that moving around
		//the scene with Scenery is vivid (move step size is fixed in Scenery), at
		//the same time we want the objects and distances to be defined with our
		//non-scaled coordinates -- so we hook everything underneath the fake object
		//that is downscaled (and consequently all is downscaled too) but defined with
		//at original scale (with original coordinates and distances)
		final float DsFactor = r.toString().contains("vulkan")? 0.04f : 0.1f;

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
		cam.perspectiveCamera(50.0f, getRenderer().getWindow().getWidth(), getRenderer().getWindow().getHeight(), 1.0f*DsFactor, 100000.0f*DsFactor);
		cam.setActive( true );
		getScene().addChild(cam);

		//a cam-attached light mini-ramp
		//------------------------------
		xCam =  0.3f*sceneSize[0] *DsFactor;
		yCam =  0.2f*sceneSize[1] *DsFactor;
		zCam = -0.1f*sceneSize[2] *DsFactor;

		float radius = 0.8f*sceneSize[1] *DsFactor;

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
		radius = 1.1f*sceneSize[1] *DsFactor;

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
			l.setIntensity((150.0f*DsFactor)*(150.0f*DsFactor));
			l.setEmissionColor(lightsColor);
		}
		for (PointLight l : fixedLights[0])
		{
			l.setIntensity((200.0f*DsFactor)*(200.0f*DsFactor));
			l.setEmissionColor(lightsColor);
		}
		for (PointLight l : fixedLights[1])
		{
			l.setIntensity((200.0f*DsFactor)*(200.0f*DsFactor));
			l.setEmissionColor(lightsColor);
		}

		//enable the fixed ramp lights
		ToggleFixedLights(); //just the front ramp
		ToggleFixedLights(); //just the rear ramp
		ToggleFixedLights(); //both ramps
	}

	/** additionally promotes SimViewer's own hot keys;
	    call this method only when the Scenery is ready... */
	void setupOwnHotkeys()
	{
		final InputHandler ih = this.getInputHandler();
		ih.addBehaviour( "SV_7", new BehaviourForFlightRecorder('7'));
		ih.addKeyBinding("SV_7", "7");
		ih.addBehaviour( "SV_8", new BehaviourForFlightRecorder('8'));
		ih.addKeyBinding("SV_8", "8");
		ih.addBehaviour( "SV_9", new BehaviourForFlightRecorder('9'));
		ih.addKeyBinding("SV_9", "9");
		ih.addBehaviour( "SV_0", new BehaviourForFlightRecorder('0'));
		ih.addKeyBinding("SV_0", "0");
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


	/** returns true when the underlying rendering machinery is ready to draw anything */
	public
	void waitUntilSceneIsReady()
	throws InterruptedException
	{
		//official Scenery flag
		while (!this.sceneInitialized()) Thread.sleep(1000);
		//proof that this.init() is indeed in progress
		while (scene == null)            Thread.sleep(1000);
		//this condition used to work alone...
		while (!scene.getInitialized())  Thread.sleep(1000);
		//also wait until InputHandler is available...
		while (this.getInputHandler() == null) Thread.sleep(1000);
	}

	/** exposes the InputHandler outside this class */
	public
	InputHandler exposeInputHandler()
	{
		return this.getInputHandler();
	}

	/** helper method to save the current content of the scene into /tmp/frameXXXX.png */
	public
	void saveNextScreenshot()
	{
		final String filename = String.format("/tmp/frame%04d.png",tickCounter);
		System.out.println("Saving screenshot: "+filename);
		this.getRenderer().screenshot(filename,true);
	}
	//
	/** flag for external modules to see if they should call saveNextScreenshot() */
	public boolean savingScreenshots = false;

	/** counts how many times the "tick message" has been received, this message
	    is assumed to be sent typically after one simulation round is over */
	private int tickCounter = 0;
	//
	public
	void increaseTickCounter()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		++tickCounter;
	 }
	}

	/** attempts to turn on/off the "push mode", and reports the state */
	public
	boolean TogglePushMode()
	{
		this.getRenderer().setPushMode( !this.getRenderer().getPushMode() );
		return this.getRenderer().getPushMode();
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
	private boolean   axesShown = false;

	public
	boolean ToggleDisplayAxes()
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

		return axesShown;
	}
	//----------------------------------------------------------------------------


	private graphics.scenery.Line[] borderData = null;
	private boolean borderShown = false;

	public
	boolean ToggleDisplaySceneBorder()
	{
		//first run, init the data
		if (borderData == null)
		{
			borderData = new graphics.scenery.Line[] {
				new graphics.scenery.Line(6), new graphics.scenery.Line(6),
				new graphics.scenery.Line(6), new graphics.scenery.Line(6)  };

			final GLVector sxsysz = new GLVector(sceneOffset[0]             , sceneOffset[1]             , sceneOffset[2]             );
			final GLVector lxsysz = new GLVector(sceneOffset[0]+sceneSize[0], sceneOffset[1]             , sceneOffset[2]             );
			final GLVector sxlysz = new GLVector(sceneOffset[0]             , sceneOffset[1]+sceneSize[1], sceneOffset[2]             );
			final GLVector lxlysz = new GLVector(sceneOffset[0]+sceneSize[0], sceneOffset[1]+sceneSize[1], sceneOffset[2]             );
			final GLVector sxsylz = new GLVector(sceneOffset[0]             , sceneOffset[1]             , sceneOffset[2]+sceneSize[2]);
			final GLVector lxsylz = new GLVector(sceneOffset[0]+sceneSize[0], sceneOffset[1]             , sceneOffset[2]+sceneSize[2]);
			final GLVector sxlylz = new GLVector(sceneOffset[0]             , sceneOffset[1]+sceneSize[1], sceneOffset[2]+sceneSize[2]);
			final GLVector lxlylz = new GLVector(sceneOffset[0]+sceneSize[0], sceneOffset[1]+sceneSize[1], sceneOffset[2]+sceneSize[2]);

			//first of the two mandatory surrounding fake points that are never displayed
			for (graphics.scenery.Line l : borderData) l.addPoint(sxsysz);

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

			for (graphics.scenery.Line l : borderData)
			{
				//second of the two mandatory surrounding fake points that are never displayed
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

		return borderShown;
	}
	//----------------------------------------------------------------------------


	/** A handle on a lock (synchronization subject) that shall be used when a caller
	    wants to modify any of the pointNodes, lineNodes, vectorNodes or tickCounter.

	    Since the DisplayScene's API for updating the displayed graphics was not
	    designed for concurrent access, the callers have to synchronize explicitly
	    among themselves. And they shall do it precisely around this attribute. */
	public final Object lockOnChangingSceneContent = new Object();

	/** these points are registered with the display, but not necessarily always visible */
	private final Map<Integer,Point> pointNodes = new HashMap<>();
	/** these lines are registered with the display, but not necessarily always visible */
	private final Map<Integer,Line> lineNodes = new HashMap<>();
	/** these vectors are registered with the display, but not necessarily always visible */
	private final Map<Integer,Vector> vectorNodes = new HashMap<>();


	/** this is designed (yet only) for SINGLE-THREAD application! */
	public
	void addUpdateOrRemovePoint(final int ID,final Point p)
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		//attempt to retrieve node of this ID
		Point n = pointNodes.get(ID);

		//negative color is an agreed signal to remove the point
		//also, get rid of a point whose radius is "impossible"
		if (p.color < 0 || p.radius.x() < 0.0f)
		{
			if (n != null)
			{
				scene.removeChild(n.node);
				pointNodes.remove(ID);
			}
			return;
		}

		//shall we create a new point?
		if (n == null)
		{
			//new point: adding
			n = new Point( new Sphere(1.0f, 12) );
			n.node.setPosition(n.centre);
			n.node.setScale(n.radius);

			pointNodes.put(ID,n);
			this.addChild(n.node);
			showOrHideMe(ID,n.node,spheresShown);
		}

		//now update the point with the current data
		n.update(p);
		n.node.setMaterial(materials[n.color % materials.length]);

		n.lastSeenTick = tickCounter;

		this.nodeSetNeedsUpdate(n.node);
	 }
	}


	/** this is designed (yet only) for SINGLE-THREAD application! */
	public
	void addUpdateOrRemoveLine(final int ID,final Line l)
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		//attempt to retrieve node of this ID
		Line n = lineNodes.get(ID);

		//negative color is an agreed signal to remove the line
		if (l.color < 0)
		{
			if (n != null)
			{
				scene.removeChild(n.node);
				lineNodes.remove(ID);
			}
			return;
		}

		//shall we create a new line?
		if (n == null)
		{
			//new line: adding
			n = new Line( new graphics.scenery.Line(4) );
			n.node.setEdgeWidth(3.0f);
			//no setPosition(), no setScale()

			lineNodes.put(ID,n);
			this.addChild(n.node);
			showOrHideMe(ID,n.node,linesShown);
		}

		//now update the line with the current data
		n.update(l);
		n.node.clearPoints();
		n.node.addPoint(zeroGLvec);
		n.node.addPoint(n.posA);
		n.node.addPoint(n.posB);
		n.node.addPoint(zeroGLvec);
		//NB: surrounded by two mandatory fake points that are never displayed
		n.node.setMaterial(materials[n.color % materials.length]);

		n.lastSeenTick = tickCounter;
	 }
	}


	/** this is designed (yet only) for SINGLE-THREAD application! */
	public
	void addUpdateOrRemoveVector(final int ID,final Vector v)
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		//attempt to retrieve node of this ID
		Vector n = vectorNodes.get(ID);

		//negative color is an agreed signal to remove the vector
		if (v.color < 0)
		{
			if (n != null)
			{
				scene.removeChild(n.node);
				vectorNodes.remove(ID);
			}
			return;
		}

		//shall we create a new vector?
		if (n == null)
		{
			//new vector: adding it already in the desired shape
			n = new Vector( new Arrow(v.vector) );
			n.node.setEdgeWidth(3.0f);
			n.node.setPosition(n.base);
			n.node.setScale(vectorsStretchGLvec);

			//(here's the "adding it" part)
			vectorNodes.put(ID,n);
			this.addChild(n.node);
			showOrHideMe(ID,n.node,vectorsShown);
		}
		else
		{
			//existing vector: update it to the desired shape
			n.node.reshape(v.vector);
		}
		//NB: n.vector is actually not used here!
		//    (as Arrow.reshape() makes its own copy of
		//     vector's shape, and we use v.vector here
		//     to shape it; no reference to n.vector is
		//     required and kept in the Arrow story)
		//
		//update the arrow with (at least) the current 'base' position
		n.update(v);
		n.node.setMaterial(materials[n.color % materials.length]);

		n.lastSeenTick = tickCounter;

		this.nodeSetNeedsUpdate(n.node);
	 }
	}


	public
	void removeAllObjects()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		tickCounter = Integer.MAX_VALUE;
		garbageCollect(-1);
	 }
	}

	public
	void garbageCollect()
	{
		garbageCollect(0);
	}

	/** remove all objects that were last touched before tickCounter-tolerance */
	public
	void garbageCollect(int tolerance)
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		//NB: HashMap may be modified while being swept through only via iterator
		//    (and iterator must remove the elements actually)
		Iterator<Integer> i = pointNodes.keySet().iterator();

		while (i.hasNext())
		{
			final Point p = pointNodes.get(i.next());

			if (p.lastSeenTick+tolerance < tickCounter)
			{
				scene.removeChild(p.node);
				i.remove();
			}
		}

		i = lineNodes.keySet().iterator();
		while (i.hasNext())
		{
			final Line l = lineNodes.get(i.next());

			if (l.lastSeenTick+tolerance < tickCounter)
			{
				scene.removeChild(l.node);
				i.remove();
			}
		}

		i = vectorNodes.keySet().iterator();
		while (i.hasNext())
		{
			final Vector v = vectorNodes.get(i.next());

			if (v.lastSeenTick+tolerance < tickCounter)
			{
				scene.removeChild(v.node);
				i.remove();
			}
		}
	 }
	}
	//
	/** flag for external modules to see if they should call garbageCollect() */
	public boolean garbageCollecting = true;
	//----------------------------------------------------------------------------


	/** flags if nodes should be scene.addChild(node)'ed and node.setNeedsUpdate(true)'ed
	    right away (the online process mode), or do all such later at once (the batch
	    process mode) because this might have positive performance impact */
	private boolean updateNodesImmediately = true;

	/** buffer of nodes to be added to the scene (ideally) at the same time */
	private final Node[] nodesYetToBeAdded    = new Node[10240]; //40 kB of RAM
	private int          nodesYetToBeAddedCnt = 0;

	/** buffer of nodes to have their 'needsUpdate' flag set (ideally) at the same time */
	private final Node[] nodesYetToBeUpdated    = new Node[10240]; //40 kB of RAM
	private int          nodesYetToBeUpdatedCnt = 0;

	/** only signals/enables the 'batch process' mode,
	    this is designed (yet only) for SINGLE-THREAD application! */
	public
	void suspendNodesUpdating()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		updateNodesImmediately = false;
	 }
	}

	/** calls processNodesYetToBeSmth() and switches back to the 'online process' mode,
	    this is designed (yet only) for SINGLE-THREAD application! */
	public
	void resumeNodesUpdating()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		updateNodesImmediately = true;
		processNodesYetToBeSmth();
	 }
	}

	/** processes (ideally at the same time) and clears the content of
	    the two buffers (buffers for adding and updating nodes simultaneously) */
	private
	void processNodesYetToBeSmth()
	{
		for (int i=0; i < nodesYetToBeAddedCnt; ++i)
			scene.addChild( nodesYetToBeAdded[i] );
		nodesYetToBeAddedCnt = 0;

		for (int i=0; i < nodesYetToBeUpdatedCnt; ++i)
			nodesYetToBeUpdated[i].setNeedsUpdate(true);
		nodesYetToBeUpdatedCnt = 0;
	}

	/** either registers the node into the Scenery's scene immediately (when in the online
	    process mode), or registers into the 'nodesYetToBeAdded' buffer (when in the batch
	    process mode) */
	private
	void addChild(final Node node)
	{
		if (updateNodesImmediately) scene.addChild(node);
		else
		{
			nodesYetToBeAdded[nodesYetToBeAddedCnt++] = node;

			//overrun protection
			if (nodesYetToBeAddedCnt == nodesYetToBeAdded.length)
				processNodesYetToBeSmth();
		}
	}

	/** either sets 'needsUpdate' flag of the node immediately (when in the online
	    process mode), or registers into the 'nodesYetToBeUpdated' buffer to
	    have it set later (when in the batch process mode) */
	private
	void nodeSetNeedsUpdate(final Node node)
	{
		if (updateNodesImmediately) node.setNeedsUpdate(true);
		else
		{
			nodesYetToBeUpdated[nodesYetToBeUpdatedCnt++] = node;

			//overrun protection
			if (nodesYetToBeUpdatedCnt == nodesYetToBeUpdated.length)
				processNodesYetToBeSmth();
		}
	}
	//----------------------------------------------------------------------------


	/** cell forces are typically small in magnitude compared to the cell size,
	    this defines the current magnification applied when displaying the force vectors */
	private float vectorsStretch = 1.f;
	private GLVector vectorsStretchGLvec = new GLVector(vectorsStretch,3);

	float getVectorsStretch()
	{ return vectorsStretch; }

	void setVectorsStretch(final float vs)
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		//update the stretch factor...
		vectorsStretch = vs;
		vectorsStretchGLvec = new GLVector(vectorsStretch,3);

		//...and rescale all vectors presently existing in the system
		vectorNodes.values().forEach( n -> n.node.setScale(vectorsStretchGLvec) );
	 }
	}

	/** shortcut zero vector to prevent from coding "new GLVector(0.f,3)" where needed */
	private final GLVector zeroGLvec = new GLVector(0.f,3);
	//----------------------------------------------------------------------------


	/** groups visibility conditions across modes for one displayed object, e.g. sphere */
	private class elementVisibility
	{
		public boolean g_Mode = true;   //the cell debug mode, operated with 'g' key
		public boolean G_Mode = true;   //the global purpose debug mode, operated with 'G'
	}

	/** signals if we want to have cells (spheres) displayed (even if cellsData is initially empty) */
	private elementVisibility spheresShown = new elementVisibility();

	/** signals if we want to have cell lines displayed */
	private elementVisibility linesShown = new elementVisibility();

	/** signals if we want to have cell forces (vectors) displayed */
	private elementVisibility vectorsShown = new elementVisibility();

	/** signals if we want to have cell "debugging" elements displayed */
	private boolean cellDebugShown = false;

	/** signals if we want to have general purpose "debugging" elements displayed */
	private boolean generalDebugShown = false;


	public
	boolean ToggleDisplayCellSpheres()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		//toggle the flag
		spheresShown.g_Mode ^= true;

		//sync expected_* constants with current state of visibility flags
		//apply the new setting on the points
		for (Integer ID : pointNodes.keySet())
			showOrHideMe(ID,pointNodes.get(ID).node,spheresShown);

		return spheresShown.g_Mode;
	 }
	}

	public
	boolean ToggleDisplayCellLines()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		linesShown.g_Mode ^= true;

		for (Integer ID : lineNodes.keySet())
			showOrHideMe(ID,lineNodes.get(ID).node,linesShown);

		return linesShown.g_Mode;
	 }
	}

	public
	boolean ToggleDisplayCellVectors()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		vectorsShown.g_Mode ^= true;

		for (Integer ID : vectorNodes.keySet())
			showOrHideMe(ID,vectorNodes.get(ID).node,vectorsShown);

		return vectorsShown.g_Mode;
	 }
	}

	public
	boolean ToggleDisplayCellDebug()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		cellDebugShown ^= true;

		//"debug" objects might be present in any shape primitive
		for (Integer ID : pointNodes.keySet())
			showOrHideMe(ID,pointNodes.get(ID).node,spheresShown);
		for (Integer ID : lineNodes.keySet())
			showOrHideMe(ID,lineNodes.get(ID).node,linesShown);
		for (Integer ID : vectorNodes.keySet())
			showOrHideMe(ID,vectorNodes.get(ID).node,vectorsShown);

		return cellDebugShown;
	 }
	}


	public
	boolean ToggleDisplayGeneralDebugSpheres()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		//toggle the flag
		spheresShown.G_Mode ^= true;

		//sync expected_* constants with current state of visibility flags
		//apply the new setting on the points
		for (Integer ID : pointNodes.keySet())
			showOrHideMe(ID,pointNodes.get(ID).node,spheresShown);

		return spheresShown.G_Mode;
	 }
	}

	public
	boolean ToggleDisplayGeneralDebugLines()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		linesShown.G_Mode ^= true;

		for (Integer ID : lineNodes.keySet())
			showOrHideMe(ID,lineNodes.get(ID).node,linesShown);

		return linesShown.G_Mode;
	 }
	}

	public
	boolean ToggleDisplayGeneralDebugVectors()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		vectorsShown.G_Mode ^= true;

		for (Integer ID : vectorNodes.keySet())
			showOrHideMe(ID,vectorNodes.get(ID).node,vectorsShown);

		return vectorsShown.G_Mode;
	 }
	}

	public
	boolean ToggleDisplayGeneralDebug()
	{
	 synchronized (lockOnChangingSceneContent)
	 {
		generalDebugShown ^= true;

		//"debug" objects might be present in any shape primitive
		for (Integer ID : pointNodes.keySet())
			showOrHideMe(ID,pointNodes.get(ID).node,spheresShown);
		for (Integer ID : lineNodes.keySet())
			showOrHideMe(ID,lineNodes.get(ID).node,linesShown);
		for (Integer ID : vectorNodes.keySet())
			showOrHideMe(ID,vectorNodes.get(ID).node,vectorsShown);

		return generalDebugShown;
	 }
	}


	public
	void EnableFrontFaceCulling()
	{
		for (Material m : materials)
			m.setCullingMode(CullingMode.Front);
	}

	public
	void DisableFrontFaceCulling()
	{
		for (Material m : materials)
			m.setCullingMode(CullingMode.None);
	}
	//----------------------------------------------------------------------------


	//ID space of graphics primitives: 31 bits
	//
	//     lowest 16 bits: ID of the graphics element itself
	//next lowest  1 bit : proper (=0) or debug (=1) element
	//next lowest 14 bits: "identification" of ONE single cell
	//     highest 1 bit : not used (sign bit)
	//
	//note: general purpose elements have cell "identification" equal to 0
	//      in which case the debug bit is not applied
	//note: there are 4 graphics primitives: points, lines, vectors, meshes;
	//      each is living in its own list of elements (e.g. this.pointNodes)

	//constants to "read out" respective information
	@SuppressWarnings("unused")
	private static final int MASK_ELEM   = ((1 << 16)-1);
	private static final int MASK_DEBUG  =   1 << 16;
	private static final int MASK_CELLID = ((1 << 14)-1) << 17;

	/** given the current display preference in 'displayFlag',
	    the visibility of the object 'n' with ID is adjusted,
	    the decided state is indicated in the return value */
	private
	boolean showOrHideMe(final int ID, final Node n, final elementVisibility displayFlag)
	{
		boolean vis = false;
		if ((ID & MASK_CELLID) == 0)
		{
			//the ID does not belong to any cell, still the (filtering) flags apply
			vis = displayFlag.G_Mode == true ? generalDebugShown : displayFlag.G_Mode;
		}
		else
		{
			vis = displayFlag.g_Mode == true && (ID & MASK_DEBUG) > 0 ?
				cellDebugShown : displayFlag.g_Mode;

			//NB: follows this table
			// flag  MASK_DEBUG   cellDebugShown    result
			// true    1          true              true
			// true    1          false             false
			// true    0          true              true
			// true    0          false             true

			// false   1          true              false
			// false   1          false             false
			// false   0          true              false
			// false   0          false             false
		}

		if (n != null) n.setVisible(vis);
		return vis;
	}
	//----------------------------------------------------------------------------


	public
	void reportSettings()
	{
		System.out.println("push mode       : " + this.getRenderer().getPushMode() + "  \tscreenshots            : " + savingScreenshots);
		System.out.println("garbage collect.: " + garbageCollecting                + "  \ttickCounter            : " + tickCounter);
		System.out.println("ambient lights  : " + fixedLightsChoosen               + "  \thead lights            : " + headLightsChoosen);
		System.out.println("scene border    : " + borderShown                      + "  \torientation compass    : " + axesShown);

		System.out.println("visibility      : 'g' 'G'"                                                               +  "\t'g' mode   (cell debug): " + cellDebugShown);
		System.out.println("         points :  "+(spheresShown.g_Mode? "Y":"N")+"   "+(spheresShown.G_Mode? "Y":"N") + " \t'G' mode (global debug): " + generalDebugShown);
		System.out.println("         lines  :  "+(  linesShown.g_Mode? "Y":"N")+"   "+(  linesShown.G_Mode? "Y":"N") + " \tvector elongation      : " + vectorsStretch + "x");
		System.out.println("         vectors:  "+(vectorsShown.g_Mode? "Y":"N")+"   "+(vectorsShown.G_Mode? "Y":"N") + " \tfront faces culling    : " + (materials[0].getCullingMode() == CullingMode.Front));

		System.out.println("number of points: " + this.pointNodes.size() + "\t  lines: "+this.lineNodes.size() + "\t  vectors: "+this.vectorNodes.size());
		System.out.println("color legend    :        white: velocity, 1stInnerMost2Yolk");
		System.out.println(" red: overlap            green: cell&skeleton          blue: friction, skelDev");
		System.out.println("cyan: body             magenta: tracks, rep&drive    yellow: slide, 2ndInnerMost2Yolk, tracksFF");
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
	//----------------------------------------------------------------------------


	/** reference on the currently available FlightRecording: the object
	    must initialized outside and reference on it is given here, otherwise
	    the reference must be null */
	CommandFromFlightRecorder flightRecorder = null;

	private class BehaviourForFlightRecorder implements ClickBehaviour
	{
		BehaviourForFlightRecorder(final char key) { actionKey = key; }
		final char actionKey;

		@Override
		public void click( final int x, final int y )
		{
			if (flightRecorder != null)
			{
				try {
					boolean status = false;
					switch (actionKey)
					{
					case '7':
						status = flightRecorder.rewindAndSendFirstTimepoint();
						break;
					case '8':
						status = flightRecorder.sendPrevTimepointMessages();
						break;
					case '9':
						status = flightRecorder.sendNextTimepointMessages();
						break;
					case '0':
						status = flightRecorder.rewindAndSendLastTimepoint();
						break;
					}

					if (!status)
						System.out.println("No FlightRecording file is opened.");
				}
				catch (InterruptedException e) {
					System.out.println("DisplayScene: Interrupted and stopping...");
					stop();
				}
			}
			else System.out.println("FlightRecording is not available.");
		}
	}
}
