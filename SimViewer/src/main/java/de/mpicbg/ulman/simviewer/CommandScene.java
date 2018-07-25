package de.mpicbg.ulman.simviewer;

import java.io.InputStreamReader;
import java.io.IOException;

import de.mpicbg.ulman.simviewer.DisplayScene;

/**
 * Adapted from TexturedCubeJavaExample.java from the scenery project,
 * originally created by kharrington on 7/6/16.
 *
 * Current version is created by Vladimir Ulman, 2018.
 */
public class CommandScene implements Runnable
{
	/** constructor to create connection to a displayed window */
	public CommandScene(final DisplayScene _scene)
	{
		scene = _scene;
	}

	/** reference on the controlled rendering display */
	private final DisplayScene scene;


	/** reads the console and dispatches the commands */
	public void run()
	{
		System.out.println("Key listener: Started.");
		final InputStreamReader console = new InputStreamReader(System.in);
		try {
			while (true)
			{
				if (console.ready())
					processKey(console.read());
				else
					Thread.sleep(1000);
			}
		}
		catch (IOException e) {
			System.out.println("Key listener: Error reading the console.");
			e.printStackTrace();
		}
		catch (InterruptedException e) {
			System.out.println("Key listener: Stopped.");
		}
	}


	private
	void processKey(final int key) throws InterruptedException
	{
		switch (key)
		{
		case 'h':
			System.out.println("List of accepted commands:");
			System.out.println("h - Shows this help message");
			System.out.println("q - Quits the program");

			System.out.println("A - Toggles display of the axes in the scene centre");
			System.out.println("B - Toggles display of the scene border");
			System.out.println("L - Toggles between front/back/both/none ramp lights");
			System.out.println("H - Toggles on/off of camera-attached lights");

			System.out.println("F - Adds some cells to have something to display");
			System.out.println("d - Deletes all cells (even if not displayed)");
			System.out.println("c - Toggles display of the cells (cell spheres)");
			System.out.println("f - Toggles display of the forces (cell vectors)");
			System.out.println("v,V - Decreases/Increases the vector display stretch");

			System.out.println("r,R - decreases/increases radius of all cells by 0.2");
			System.out.println("x,X - moves left/right x-position of all cells by 0.2");
			break;

		case 'A':
			scene.ToggleDisplayAxes();
			break;
		case 'B':
			scene.ToggleDisplaySceneBorder();
			break;
		case 'L':
			System.out.println("Current ramp lights: "+scene.ToggleFixedLights());
			break;
		case 'H':
			System.out.println("Current head lights: "+scene.ToggleHeadLights());
			break;

		case 'F':
			CreateFakeCells();
			break;
		case 'd':
			scene.RemoveCells();
			break;
		case 'c':
			scene.ToggleDisplayCellGeom();
			break;

		case 'f':
			//scene.ToggleDisplayVectors();
			break;
		case 'v':
			scene.setVectorsStretch(0.80f * scene.getVectorsStretch());
			System.out.println("new vector stretch: "+scene.getVectorsStretch());
			break;
		case 'V':
			scene.setVectorsStretch(1.25f * scene.getVectorsStretch());
			System.out.println("new vector stretch: "+scene.getVectorsStretch());
			break;

		/*
		case 'r':
			for (Cell c : scene.cellsData.values())
			{
				for (int i=0; i < c.sphereRadii.length; ++i)
					c.sphereRadii[i] -= 0.2f;
				scene.UpdateCellSphereNodes(c);
			}
			break;
		case 'R':
			for (Cell c : scene.cellsData.values())
			{
				for (int i=0; i < c.sphereRadii.length; ++i)
					c.sphereRadii[i] += 0.2f;
				scene.UpdateCellSphereNodes(c);
			}
			break;

		case 'x':
			for (Cell c : scene.cellsData.values())
			{
				for (int i=0; i < c.sphereRadii.length; ++i)
					c.sphereCentres[3*i +0] -= 0.2f;
				scene.UpdateCellSphereNodes(c);
			}
			break;
		case 'X':
			for (Cell c : scene.cellsData.values())
			{
				for (int i=0; i < c.sphereRadii.length; ++i)
					c.sphereCentres[3*i +0] += 0.2f;
				scene.UpdateCellSphereNodes(c);
			}
			break;
		*/

		case 'q':
			scene.stop();
			//don't wait for GUI to tell me to stop
			throw new InterruptedException("Stop myself now.");
		}
	}


	void CreateFakeCells()
	{
		final float xStep = scene.sceneSize[0] / 6.0f;
		final float yStep = scene.sceneSize[1] / 6.0f;

		final float xCentre  = scene.sceneOffset[0] + 0.5f*scene.sceneSize[0];
		final float yCentre  = scene.sceneOffset[1] + 0.5f*scene.sceneSize[1];
		final float zCentre  = scene.sceneOffset[2] + 0.5f*scene.sceneSize[2];

		final DisplayScene.myPoint c = scene.new myPoint();
		int ID = 0;

		for (int y=0; y < 5; ++y)
		for (int x=0; x < 5; ++x)
		{
			//if (x != 2 && y != 2)
			{
				ID = ((x+10*y) << 48) +1;
				c.centre[0] = xCentre + xStep*(x-2.0f) -2.0f;
				c.centre[1] = yCentre + yStep*(y-2.0f);
				c.centre[2] = zCentre - 1.0f;
				c.radius  = 3.0f;
				c.color = 2;
				scene.addUpdateOrRemovePoint(ID,c);

				ID = ((x+10*y) << 48) +2;
				c.centre[0] = xCentre + xStep*(x-2.0f) +2.0f;
				c.centre[1] = yCentre + yStep*(y-2.0f);
				c.centre[2] = zCentre + 1.0f;
				c.radius  = 3.0f;
				c.color = 3;
				scene.addUpdateOrRemovePoint(ID,c);
			}
		}
	}
}
