package de.mpicbg.ulman.simviewer;

import cleargl.GLVector;
import graphics.scenery.*;
import graphics.scenery.backends.Renderer;

/**
 * Adapted from TexturedCubeJavaExample.java from the scenery project,
 * originally created by kharrington on 7/6/16.
 *
 * Current version is created by Vladimir Ulman, 2018.
 */
public class DisplayScene extends SceneryBase
{
	/** constructor to create an empty window */
	public DisplayScene(final float[] sOffset, final float[] sSize)
	{
		//the last param determines whether REPL is wanted, or not
		super("EmbryoGen - live view (using scenery)", 768,768, false);

		if (sOffset.length != 3 || sSize.length != 3)
			throw new RuntimeException("Offset and Size must be 3-items long: 3D world!");

		sceneOffset = sOffset.clone();
		sceneSize   = sSize.clone();
	}

	/** 3D position of the scene, to position well the lights and camera */
	final float[] sceneOffset;
	/** 3D size (diagonal vector) of the scene, to position well the lights and camera */
	final float[] sceneSize;


	/** initializes the scene with one camera and multiple lights */
	public void init()
	{
		setRenderer( Renderer.createRenderer(getHub(), getApplicationName(), getScene(), getWindowWidth(), getWindowHeight()));
		getHub().add(SceneryElement.Renderer, getRenderer());

		PointLight[] lights = new PointLight[]
			{ new PointLight(15.0f), new PointLight(15.0f),
			  new PointLight(15.0f), new PointLight(15.0f) };

		final float[] sceneBackCorner = new float[3];
		sceneBackCorner[0] = sceneOffset[0] + sceneSize[0];
		sceneBackCorner[1] = sceneOffset[1] + sceneSize[1];
		sceneBackCorner[2] = sceneOffset[2] + sceneSize[2];

		lights[0].setPosition(new GLVector(sceneOffset[0]    ,sceneOffset[1]    ,sceneBackCorner[2]));
		lights[1].setPosition(new GLVector(sceneBackCorner[0],sceneOffset[1]    ,sceneBackCorner[2]));
		lights[2].setPosition(new GLVector(sceneOffset[0]    ,sceneBackCorner[1],sceneBackCorner[2]));
		lights[3].setPosition(new GLVector(sceneBackCorner[0],sceneBackCorner[1],sceneBackCorner[2]));

		for (PointLight l : lights)
		{
			l.setIntensity(100.0f);
			l.setEmissionColor(new GLVector(1.0f, 1.0f, 1.0f));
			getScene().addChild(l);
		}

		sceneBackCorner[0] = sceneOffset[0] + 0.5f*sceneSize[0];
		sceneBackCorner[1] = sceneOffset[1] + 0.5f*sceneSize[1];
		sceneBackCorner[2] = sceneOffset[2] + 3.0f*sceneSize[2];

		Camera cam = new DetachedHeadCamera();
		cam.setPosition( new GLVector(sceneBackCorner[0],sceneBackCorner[1],sceneBackCorner[2]) );
		cam.perspectiveCamera(50.0f, getRenderer().getWindow().getWidth(), getRenderer().getWindow().getHeight(), 0.1f, 1000.0f);
		cam.setActive( true );
		getScene().addChild(cam);
	}


	public void UpdateCell()
	{
		Material material = new Material();
		material.setAmbient( new GLVector(1.0f, 0.0f, 0.0f) );
		material.setDiffuse( new GLVector(0.0f, 1.0f, 0.0f) );
		material.setSpecular( new GLVector(1.0f, 1.0f, 1.0f) );

		final Sphere sph1 = new Sphere(1.0f, 12);
		sph1.setMaterial( material );
		sph1.setPosition( new GLVector(0.0f, 0.0f, 0.0f) );
		//System.out.println("Sphere is made of "+sph1.getVertices().capacity()+" vertices");
		getScene().addChild(sph1);
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
	}






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
