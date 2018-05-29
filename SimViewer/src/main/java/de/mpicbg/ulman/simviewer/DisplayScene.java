package de.mpicbg.ulman.simviewer;

import cleargl.GLVector;
import graphics.scenery.*;
import graphics.scenery.backends.Renderer;

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
		super("EmbryoGen - live view using scenery", 768,768, false);

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

	/** short cut to Scene, instead of calling getScene() */
	Scene scene;

	/** initializes the scene with one camera and multiple lights */
	public
	void init()
	{
		scene = getScene();

		setRenderer( Renderer.createRenderer(getHub(), getApplicationName(), scene, getWindowWidth(), getWindowHeight()));
		getHub().add(SceneryElement.Renderer, getRenderer());

		//calculate back corner of the scene
		final float[] sceneSomeCorner = new float[3];
		sceneSomeCorner[0] = sceneOffset[0] +      sceneSize[0];
		sceneSomeCorner[1] = sceneOffset[1] +      sceneSize[1];
		sceneSomeCorner[2] = sceneOffset[2] + 1.2f*sceneSize[2];

		float radius = sceneSize[0]*sceneSize[0] +
		               sceneSize[1]*sceneSize[1] +
		               sceneSize[2]*sceneSize[2];
		radius = 1.5f * (float)Math.sqrt(radius);

		//create the lights, one for each upper corner of the scene
		final PointLight[] lights = new PointLight[]
			{ new PointLight(radius), new PointLight(radius),
			  new PointLight(radius), new PointLight(radius) };

		//position specifically to the corners
		lights[0].setPosition(new GLVector(sceneOffset[0]    ,sceneOffset[1]    ,sceneSomeCorner[2]));
		lights[1].setPosition(new GLVector(sceneSomeCorner[0],sceneOffset[1]    ,sceneSomeCorner[2]));
		lights[2].setPosition(new GLVector(sceneOffset[0]    ,sceneSomeCorner[1],sceneSomeCorner[2]));
		lights[3].setPosition(new GLVector(sceneSomeCorner[0],sceneSomeCorner[1],sceneSomeCorner[2]));

		//common settings of the lights
		for (PointLight l : lights)
		{
			l.setIntensity(3000.0f);
			l.setEmissionColor(new GLVector(1.0f, 1.0f, 1.0f));
			scene.addChild(l);
		}

		//camera position, looking from the top into the scene centre
		//NB: z-position should be such that the FOV covers the whole scene
		sceneSomeCorner[0] = sceneOffset[0] + 0.5f*sceneSize[0];
		sceneSomeCorner[1] = sceneOffset[1] + 0.5f*sceneSize[1];
		sceneSomeCorner[2] = sceneOffset[2] + 1.6f*sceneSize[2];

		Camera cam = new DetachedHeadCamera();
		cam.setPosition( new GLVector(sceneSomeCorner[0],sceneSomeCorner[1],sceneSomeCorner[2]) );
		cam.perspectiveCamera(50.0f, getRenderer().getWindow().getWidth(), getRenderer().getWindow().getHeight(), 0.1f, 1000.0f);
		cam.setActive( true );
		scene.addChild(cam);
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


	private Cylinder[] axesData = null;
	private boolean    axesShown = false;

	public
	void ToggleDisplayAxes()
	{
		//first run, init the data
		if (axesData == null)
		{
			axesData = new Cylinder[] {
				new Cylinder(0.1f,2.0f,4),
				new Cylinder(0.1f,2.0f,4),
				new Cylinder(0.1f,2.0f,4)};

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
				sceneOffset[0] + 0.5f*sceneSize[0],
				sceneOffset[1] + 0.5f*sceneSize[1],
				sceneOffset[2] + 0.5f*sceneSize[2]);
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


	private Line[]  borderData = null;
	private boolean borderShown = false;

	public
	void ToggleDisplaySceneBorder()
	{
		//first run, init the data
		if (borderData == null)
		{
			borderData = new Line[] {
				new Line(4), new Line(4),
				new Line(4), new Line(4)};

			final GLVector xEdge = new GLVector(sceneSize[0],0.0f,0.0f);
			final GLVector yEdge = new GLVector(0.0f,sceneSize[1],0.0f);
			final GLVector zEdge = new GLVector(0.0f,0.0f,sceneSize[2]);

			GLVector corner = new GLVector(-1.0f,-1.0f,0.0f);
			borderData[0].addPoint(corner);
			borderData[0].addPoint(new GLVector(-1.0f,+1.0f,0.0f));
			borderData[0].addPoint(new GLVector(+1.0f,+1.0f,0.0f));
			borderData[0].addPoint(new GLVector(+1.0f,-1.0f,0.0f));
			borderData[0].setPosition(corner);

			//TODO: add correct edges of the scene box

			for (Line l : borderData)
			{
				l.setEdgeWidth(2.0f);
				l.setMaterial(materials[0]);
			}
		}

		//add-or-remove from the scene
		for (Node n : borderData)
			if (borderShown) scene.removeChild(n);
			else             scene.addChild(n);

		//toggle the flag
		borderShown ^= true;
	}


	/** these guys will be displayed/treated on the display */
	public Map<Integer,Cell> cellsData  = new HashMap<Integer,Cell>();
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

	/** this one injects shape object(s) into the a cell */
	public
	void UpdateCellNodes(final Cell c)
	{
		//update the sphere Nodes
		int i=0;
		for (; i < c.sphereRadii.length; ++i)
		{
			if (c.sphereNodes[i] == null)
			{
				//create and enable a new display element (Node)
				c.sphereNodes[i] = new Sphere(1.0f, 12);
				scene.addChild(c.sphereNodes[i]);
			}

			//update...
			c.sphereNodes[i].setScale(new GLVector(c.sphereRadii[i],3));
			c.sphereNodes[i].setPosition(new GLVector(c.sphereCentres[3*i+0],
			                                          c.sphereCentres[3*i+1],
			                                          c.sphereCentres[3*i+2]));
			c.sphereNodes[i].setMaterial(materials[c.sphereColors[i] % materials.length]);
		}

		//disable and remove not-used Nodes
		for (; i < c.sphereNodes.length; ++i)
		if (c.sphereNodes[i] != null)
		{
			scene.removeChild(c.sphereNodes[i]);
			c.sphereNodes[i] = null;
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
