package de.mpicbg.ulman.simviewer;

import java.util.HashMap;
import de.mpicbg.ulman.simviewer.agents.Cell;
import de.mpicbg.ulman.simviewer.DisplayScene;
import de.mpicbg.ulman.simviewer.CommandScene;

/**
 * Opens the scenery window, starts the listening server, maintains
 * lightweight vector-graphics representation of cells and force vectors
 * acting on them, and maintains history of this representation (so that
 * the evolution of the population can be re-played).
 *
 * There will be a couple of threads:
 * - one to host SceneryBase.main() -- the loop that governs the display,
 *   and displays from data structures
 * - one to interact on console with the user to control Scenery
 * - one to host the ZeroMQ server to listen for stuff,
 *   and update the data structures
 * 
 * Created by Vladimir Ulman, 2018.
 */
public class StartUpScene
{
	public static void main(String... args)
	{
		DisplayScene scene = new DisplayScene(new float[] {-10.f,-10.f,-10.f},
		                                      new float[] { 20.f, 20.f, 20.f});

		final Thread GUIwindow  = new Thread(scene);
		final Thread GUIcontrol = new Thread(new CommandScene(scene));

		try {
			//start the rendering window and both window controls (console and network)
			GUIwindow.start();
			GUIcontrol.start();
			//Network.start();

			//give the GUI window some time to settle down, and populate it
			Thread.sleep(5000);

			CreateFakeCells(scene);
			for (Cell c : scene.cellsData.values())
				scene.UpdateCellNodes(c);


			//adds another group of cells a bit later
			Thread.sleep(20000);
			CreateFakeCells2(scene);
			for (Cell c : scene.cellsData.values())
				scene.UpdateCellNodes(c);


			//how this can be stopped?
			//network shall never stop by itself, it should keep reading and updating structures
			//control shall never stop unless 'stop key' is hit in which case it signals GUI to stop
			//GUI can stop anytime (either from 'stop key' or just by user closing the window)

			//wait for the GUI window to finish
			GUIwindow.join();

			//signal the remaining threads to stop
			if (GUIcontrol.isAlive()) GUIcontrol.interrupt();
			//Network.interrupt();
		} catch (InterruptedException e) {
			System.out.println("We've been interrupted while waiting for our threads to close...");
			e.printStackTrace();
		}
	}


	public static void CreateFakeCells(final DisplayScene scene)
	{
		scene.cellsData = new HashMap<Integer,Cell>();

		for (int y=0; y < 5; ++y)
		for (int x=0; x < 5; ++x)
		{
			if (x != 2 && y != 2)
			{
				final Cell c = new Cell(2,0);
				c.ID = x+10*y;
				c.sphereRadii[0]  = 1.0f;
				c.sphereColors[0] = 2;
				c.sphereCentres[0] = 3.3f*(x-2.0f);
				c.sphereCentres[1] = 3.3f*(y-2.0f);
				c.sphereCentres[2] = 0.0f;

				c.sphereRadii[1]  = 1.0f;
				c.sphereColors[1] = 3;
				c.sphereCentres[3] = 3.3f*(x-2.0f) +0.7f;
				c.sphereCentres[4] = 3.3f*(y-2.0f);
				c.sphereCentres[5] = 0.0f;

				scene.cellsData.put(c.ID, c);
			}
		}
	}
	public static void CreateFakeCells2(final DisplayScene scene)
	{

		for (int y=0; y < 5; ++y)
		for (int x=0; x < 5; ++x)
		{
			if (x == 2 || y == 2)
			{
				final Cell c = new Cell(4,0);
				c.ID = x+10*y;
				c.sphereRadii[0]  = 1.0f;
				c.sphereColors[0] = 4;
				c.sphereCentres[0] = 3.3f*(x-2.0f);
				c.sphereCentres[1] = 3.3f*(y-2.0f);
				c.sphereCentres[2] = 0.0f;

				c.sphereRadii[1]  = 1.0f;
				c.sphereColors[1] = 5;
				c.sphereCentres[3] = 3.3f*(x-2.0f) +0.7f;
				c.sphereCentres[4] = 3.3f*(y-2.0f);
				c.sphereCentres[5] = 0.0f;

				scene.cellsData.put(c.ID, c);
			}
		}
	}
}
