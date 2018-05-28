package de.mpicbg.ulman.simviewer;

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

		//start the 3 threads here
		GUIwindow.start();
		GUIcontrol.start();
		//Network.start();

		//how this can be stopped?
		//network shall never stop by itself, it should keep reading and updating structures
		//control shall never stop unless 'stop key' is hit in which case it signals GUI to stop
		//GUI can stop anytime (either from 'stop key' or just by closing the window)

		try {
			//wait for the GUI window to finish
			GUIwindow.join();

			//signal the remaining threads to stop
			GUIcontrol.interrupt();
			//Network.interrupt();
		} catch (InterruptedException e) {
			System.out.println("We've been interrupted while waiting for our threads to close...");
			e.printStackTrace();
		}
	}
}
