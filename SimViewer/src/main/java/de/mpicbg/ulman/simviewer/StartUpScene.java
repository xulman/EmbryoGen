package de.mpicbg.ulman.simviewer;

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
		DisplayScene scene = new DisplayScene(new float[] {  0.f,  0.f,  0.f},
		                                      new float[] {480.f,220.f,220.f});

		final Thread GUIwindow  = new Thread(scene);
		final Thread GUIcontrol = new Thread(new CommandScene(scene));
		final Thread Network    = new Thread(new NetworkScene(scene));

		try {
			//start the rendering window first
			GUIwindow.start();

			//give the GUI window some time to settle down, and populate it
			Thread.sleep(5000);
			while (!scene.scene.getInitialized()) Thread.sleep(3000);
			System.out.println("SimViewer is ready!");

			//only now start the both window controls (console and network)
			GUIcontrol.start();
			Network.start();

			//how this can be stopped?
			//network shall never stop by itself, it should keep reading and updating structures
			//control shall never stop unless 'stop key' is hit in which case it signals GUI to stop
			//GUI can stop anytime (either from 'stop key' or just by user closing the window)

			//wait for the GUI window to finish
			GUIwindow.join();

			//signal the remaining threads to stop
			if (GUIcontrol.isAlive()) GUIcontrol.interrupt();
			if (Network.isAlive())    Network.interrupt();
		} catch (InterruptedException e) {
			System.out.println("We've been interrupted while waiting for our threads to close...");
			e.printStackTrace();
		}
	}
}
