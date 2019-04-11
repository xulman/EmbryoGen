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
 * This file was created and is being developed by Vladimir Ulman, 2018.
 */
public class StartUpScene
{
	public static void main(String... args)
	{
		DisplayScene scene = new DisplayScene(new float[] {  0.f,  0.f,  0.f},
		                                      new float[] {480.f,220.f,220.f});

		final Thread GUIwindow  = new Thread(scene);
		try {
			String initSequence = null;
			int receivingPort = 8765;

			int parsedPort1 = -2365, parsedPort2 = -2365;
			try {
				if (args.length > 0) parsedPort1 = Integer.parseUnsignedInt(args[0]);
			}
			catch (NumberFormatException e) { /* just don't stop here... */ }
			//
			try {
				if (args.length > 1) parsedPort2 = Integer.parseUnsignedInt(args[1]);
			}
			catch (NumberFormatException e) { /* just don't stop here... */ }
			//
			if (parsedPort1 > 1024)
			{
				receivingPort = parsedPort1;
				if (args.length > 1) initSequence = args[1];
			}
			else if (parsedPort2 > 1024)
			{
				receivingPort = parsedPort2;
				initSequence = args[0];
			}
			else if (args.length > 0) initSequence = args[0];

			//start the rendering window first
			GUIwindow.start();

			//give the GUI window some time to settle down, and populate it
			Thread.sleep(5000);
			while (!scene.scene.getInitialized()) Thread.sleep(3000);
			System.out.println("SimViewer is ready!");

			//only now start the both window controls (console and network)
			final Thread GUIcontrol = new Thread(new CommandScene(scene, initSequence));
			final Thread Network    = new Thread(new NetworkScene(scene, receivingPort));
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
