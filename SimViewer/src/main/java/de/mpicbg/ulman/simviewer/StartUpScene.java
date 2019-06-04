package de.mpicbg.ulman.simviewer;

import de.mpicbg.ulman.simviewer.aux.NetMessagesProcessor;

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
 * Adapted from TexturedCubeJavaExample.java from the scenery project,
 * originally created by kharrington on 7/6/16.
 *
 * This file was created and is being developed by Vladimir Ulman, 2018.
 */
public class StartUpScene
{
	public static void main(String... args)
	{
		//setup the SimViewer's GUI window and start it
		DisplayScene scene = new DisplayScene(new float[] {  0.f,  0.f,  0.f},
		                                      new float[] {480.f,220.f,220.f});

		final Thread GUIwindow = new Thread(scene);
		GUIwindow.start();

		try {
			//try to understand the command line options if there are some,
			//the options can be (in any order):
			// - single integer port number not smaller than 1025
			// - sequence of (initial) "commands" that CommandFromCLI can
			//   recognize with no white spaces inbetween
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

			//give the GUI window some time to settle down, and populate it
			scene.waitUntilSceneIsReady();
			System.out.println("SimViewer is ready!");

			//init the remaining controls:
			//extra hot keys to be registered with the 'scene'
			scene.setupOwnHotkeys();

			//the shared, messages processor and its "wrapping classes"
			final NetMessagesProcessor netMsgProcessor = new NetMessagesProcessor(scene);
			//
			final CommandFromNetwork        cmdNet = new CommandFromNetwork(netMsgProcessor, receivingPort);
			final CommandFromFlightRecorder cmdFR  = new CommandFromFlightRecorder(netMsgProcessor);

			//the user-commands processor
			final CommandFromCLI            cmdCLI = new CommandFromCLI(scene, initSequence);

			//notes:
			//cmdNet recieves its messages from a network and it is this event that triggers its activity
			//cmdFR recieves its messages from a file and it the user ('scene' or cmdCLI) that triggers its activity
			//
			//cmdCLI interacts with the 'scene' directly for some of its functionalities, however,
			//functionality related to the FlightRecording is outsourced to the cmdFR
			//
			//cmdNet and cmdCLI are therefore "living" in separate threads (coded later)
			//
			//the cmdCLI as well as the 'scene' itself can also influence/command the cmdFR (they trigger its activity),
			//and so we "inject" the reference on it
			cmdCLI.flightRecorder = cmdFR;
			 scene.flightRecorder = cmdFR;

			//only now start the additional controls (console and network)
			final Thread CLIcontrol = new Thread( cmdCLI );
			final Thread NETcontrol = new Thread( cmdNet );
			CLIcontrol.start();
			NETcontrol.start();

			//how this can be stopped?
			//network shall never stop by itself, it should keep reading and updating structures
			//control shall never stop unless 'stop key' is hit in which case it signals GUI to stop
			//GUI can stop anytime (either from 'stop key' or just by user closing the window)

			//wait for the GUI window to finish
			GUIwindow.join();

			//signal the remaining threads to stop
			if (CLIcontrol.isAlive()) CLIcontrol.interrupt();
			if (NETcontrol.isAlive()) NETcontrol.interrupt();
		}
		catch (InterruptedException e) {
			System.out.println("We've been interrupted while waiting for our threads to close...");
			e.printStackTrace();
		}
	}
}
