/*
 * CC BY-SA 4.0
 *
 * The code is licensed with "Attribution-ShareAlike 4.0 International license".
 * See the license details:
 *     https://creativecommons.org/licenses/by-sa/4.0/
 *
 * Copyright (C) 2019 Vladimír Ulman
 */
package de.mpicbg.ulman.simviewer_FijiPlugin;

import org.scijava.command.Command;
import org.scijava.plugin.Plugin;

@Plugin(type = Command.class, menuPath = "Plugins>EmbryoGen>SimViewer",
        name = "EmbryoGen_SimViewerPlugin", headless = true,
		  description = "Fires up the de.mpicbg.ulman.simviewer, a Scenery viewer of the running EmbryoGen simulation.")
public class simviewer_FijiPlugin implements Command
{
	@Override
	public void run()
	{
		runSimViewer();
	}

	public static
	void runSimViewer()
	{
		System.out.println("starting the EmbryoGen's SimViewer...");
		de.mpicbg.ulman.simviewer.StartUpScene.main();
	}

	public static void main(final String... args)
	{
		//start up our own ImageJ instance
		new net.imagej.ImageJ(); //.showUI();
		runSimViewer();
	}
}
