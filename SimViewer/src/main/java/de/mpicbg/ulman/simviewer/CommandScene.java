package de.mpicbg.ulman.simviewer;

import java.io.InputStreamReader;
import java.io.IOException;

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
			System.out.println("I - Toggles between front/back/both/none ramp lights");
			System.out.println("H - Toggles on/off of camera-attached lights");
			System.out.println("r,R - Asks Scenery to re-render only-update-signalling/all objects");
			System.out.println("s - Saves the current content as a screenshot image");
			System.out.println("S - Toggles automatic saving of screenshots (always after vectors update)");

			System.out.println("P - Adds some cells to have something to display");
			System.out.println("D - Deletes all objects (even if not displayed)");
			System.out.println("c,C - Toggles display of the cell/general-debug spheres (shape)");
			System.out.println("l,L - Toggles display of the cell/general-debug lines");
			System.out.println("f,F - Toggles display of the cell/general-debug vectors (forces)");
			System.out.println("g,G - Toggles display of the cell-debug/general-debug");
			System.out.println("m,M - Disable/Enable culling of front faces (Display/Hide)");
			System.out.println("v,V - Decreases/Increases the vector display stretch");
			break;

		case 'A':
			System.out.println("Axes displayed: "+scene.ToggleDisplayAxes());
			break;
		case 'B':
			System.out.println("Scene border displayed: "+scene.ToggleDisplaySceneBorder());
			break;
		case 'I':
			System.out.println("Current ramp lights: "+scene.ToggleFixedLights());
			break;
		case 'H':
			System.out.println("Current head lights: "+scene.ToggleHeadLights());
			break;

		case 'P':
			CreateFakeCells();
			System.out.println("Fake cells added");
			break;
		case 'D':
			scene.RemoveAllObjects();
			System.out.println("All objects removed (incl. lines and vectors)");
			break;

		case 'c':
			System.out.println("Cell spheres displayed: "+scene.ToggleDisplayCellSpheres());
			break;
		case 'l':
			System.out.println("Cell lines displayed: "+scene.ToggleDisplayCellLines());
			break;
		case 'f':
			System.out.println("Cell vectors displayed: "+scene.ToggleDisplayCellVectors());
			break;

		case 'C':
			System.out.println("General debug spheres displayed: "+scene.ToggleDisplayGeneralDebugSpheres());
			break;
		case 'L':
			System.out.println("General debug lines displayed: "+scene.ToggleDisplayGeneralDebugLines());
			break;
		case 'F':
			System.out.println("General debug vectors displayed: "+scene.ToggleDisplayGeneralDebugVectors());
			break;

		case 'v':
			scene.setVectorsStretch(0.80f * scene.getVectorsStretch());
			System.out.println("new vector stretch: "+scene.getVectorsStretch());
			break;
		case 'V':
			scene.setVectorsStretch(1.25f * scene.getVectorsStretch());
			System.out.println("new vector stretch: "+scene.getVectorsStretch());
			break;

		case 'g':
			System.out.println("Cell debug displayed: "+scene.ToggleDisplayCellDebug());
			break;
		case 'G':
			System.out.println("General debug displayed: "+scene.ToggleDisplayGeneralDebug());
			break;

		case 'm':
			scene.DisableFrontFaceCulling();
			System.out.println("Front faces displayed");
			break;
		case 'M':
			scene.EnableFrontFaceCulling();
			System.out.println("Front faces NOT displayed");
			break;

		case 'r':
			scene.scene.updateWorld(true, false);
			System.out.println("Scenery refreshed (only those that flag 'update needed')");
			break;
		case 'R':
			scene.scene.updateWorld(true, true);
			System.out.println("Scenery refreshed (forcily everyone)");
			break;
		case 's':
			scene.saveNextScreenshot();
			System.out.println("Current content (screenshot) just saved into a file");
			break;
		case 'S':
			scene.savingScreenshots ^= true;
			System.out.println("Automatic screenshots are now: "+scene.savingScreenshots);
			break;

		case 'q':
			scene.stop();
			//don't wait for GUI to tell me to stop
			throw new InterruptedException("Stop myself now.");
		default:
			if (key != '\n') //do not respond to Enter keystrokes
				System.out.println("Not recognized command, no action taken");
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
				ID = (x+10*y +1) << 17; //cell ID
				ID++;                   //1st element of this cell
				c.centre.set(0, xCentre + xStep*(x-2.0f) -2.0f);
				c.centre.set(1, yCentre + yStep*(y-2.0f));
				c.centre.set(2, zCentre - 1.0f);
				c.radius.set(0, 3.0f);
				c.radius.set(1, 3.0f);
				c.radius.set(2, 3.0f);
				c.color = 2;
				scene.addUpdateOrRemovePoint(ID,c);

				ID++;                   //2nd element of this cell
				c.centre.set(0, xCentre + xStep*(x-2.0f) +2.0f);
				c.centre.set(1, yCentre + yStep*(y-2.0f));
				c.centre.set(2, zCentre + 1.0f);
				c.radius.set(0, 3.0f);
				c.radius.set(1, 3.0f);
				c.radius.set(2, 3.0f);
				c.color = 3;
				scene.addUpdateOrRemovePoint(ID,c);
			}
		}
	}
}
