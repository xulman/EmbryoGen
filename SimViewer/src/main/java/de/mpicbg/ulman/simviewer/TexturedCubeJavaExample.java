package de.mpicbg.ulman.simviewer;

import cleargl.GLVector;
import graphics.scenery.*;
import graphics.scenery.backends.Renderer;

/**
 * Created by kharrington on 7/6/16.
 */
public class TexturedCubeJavaExample
{
	public static void main(String... args)
	{
		TexturedCubeJavaExample example = new TexturedCubeJavaExample();
        TexturedCubeJavaApplication viewer = example.new TexturedCubeJavaApplication( "scenery - TexturedCubeExample", 512, 512);
        //this is the main loop: the program stays here until the main window is closed
        viewer.main();
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

    private class TexturedCubeJavaApplication extends SceneryBase
    {
        public TexturedCubeJavaApplication(String applicationName, int windowWidth, int windowHeight)
        {
            //the last param determines whether REPL is wanted, or not
            super(applicationName, windowWidth, windowHeight, false);
        }

        public void init()
        {
            setRenderer( Renderer.createRenderer(getHub(), getApplicationName(), getScene(), getWindowWidth(), getWindowHeight()));
            getHub().add(SceneryElement.Renderer, getRenderer());

            Material boxmaterial = new Material();
            boxmaterial.setAmbient( new GLVector(1.0f, 0.0f, 0.0f) );
            boxmaterial.setDiffuse( new GLVector(0.0f, 1.0f, 0.0f) );
            boxmaterial.setSpecular( new GLVector(1.0f, 1.0f, 1.0f) );
            //boxmaterial.getTextures().put("diffuse", TexturedCubeJavaApplication.class.getResource("textures/helix.png").getFile() );

            /*
            final Box box = new Box(new GLVector(1.0f, 1.0f, 1.0f), false);
            box.setMaterial( boxmaterial );
            box.setPosition( new GLVector(0.0f, 0.0f, 0.0f) );
            getScene().addChild(box);
            */

            final Sphere sph1 = new Sphere(1.0f, 12);
            sph1.setMaterial( boxmaterial );
            sph1.setPosition( new GLVector(0.0f, 0.0f, 0.0f) );
            //System.out.println("Sphere is made of "+sph1.getVertices().capacity()+" vertices");
            getScene().addChild(sph1);
            //getScene().removeChild(sph1);

            /*
            //the argument to the constructor is how many vertices times 3 the line should be made of
            final Line ln1 = new Line(9);
            GLVector vec1 = new GLVector(2.0f,0.0f,0.0f);
            //ln1.addPoint(sph1.getPosition());
            //ln1.addPoint(sph1.getPosition().plus(vec1));
            ln1.setMaterial(boxmaterial);
            ln1.addPoint(new GLVector(0.0f,0.0f,0.0f));
            ln1.addPoint(new GLVector(2.0f,0.0f,0.0f));
            ln1.addPoint(new GLVector(2.0f,2.0f,0.0f));
            ln1.setEdgeWidth(5);
            ln1.setPosition(new GLVector(0.0f,0.0f,0.0f));
            getScene().addChild(ln1);
            */

            final Cylinder cyl1 = new Cylinder(0.1f,2.0f,4);
            cyl1.setMaterial(boxmaterial);
            //cyl1.setPosition(new GLVector(1.1f,0.0f,0.0f)); //default is zero
            sph1.addChild(cyl1);

            //here, the order of getRotation() and setPosition() does not matter
            //(seems that it rotates always around object's own center, rotates the model)
            //
            //if object is child to another object, the setPosition() is relative to the
            //parent object's position


            PointLight light = new PointLight(15.0f);
            light.setPosition(new GLVector(0.0f, 0.0f, 2.0f));
            light.setIntensity(100.0f);
            light.setEmissionColor(new GLVector(1.0f, 1.0f, 1.0f));
            getScene().addChild(light);
            //this is the current orientation of the cylinder's main axis
            final GLVector mainAxis1 = new GLVector(0.0f,1.0f,0.0f); //NB: already normalized

            //this is the vector that we aim to display as a narrow cylinder
            GLVector vec1 = new GLVector(2.0f,0.0f,0.0f);
            ReOrientNode(cyl1,mainAxis1,vec1);
            System.out.println("main axis=("+mainAxis1.x()+","+mainAxis1.y()+","+mainAxis1.z()+")");

            //rotate again just to see that re-orienting works...
            vec1.set(1, 2.0f);
            ReOrientNode(cyl1,mainAxis1,vec1);
            System.out.println("main axis=("+mainAxis1.x()+","+mainAxis1.y()+","+mainAxis1.z()+")");


            Camera cam = new DetachedHeadCamera();
            cam.setPosition( new GLVector(0.0f, 0.0f, 5.0f) );
            cam.perspectiveCamera(50.0f, getRenderer().getWindow().getWidth(), getRenderer().getWindow().getHeight(), 0.1f, 1000.0f);
            cam.setActive( true );
            getScene().addChild(cam);

            /*
            Thread rotator = new Thread(){
                public void run() {
                    while (true) {
                        box.getRotation().rotateByAngleY(0.01f);
                        box.setNeedsUpdate(true);

                        try {
                            Thread.sleep(20);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
            };
            rotator.start();
            */
        }
    }

}
