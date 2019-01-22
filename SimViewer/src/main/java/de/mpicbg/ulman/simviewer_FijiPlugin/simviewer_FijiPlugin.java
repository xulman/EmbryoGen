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
import org.scijava.plugin.Parameter;
import org.scijava.plugin.Plugin;
import org.scijava.log.LogService;
import net.imagej.ImageJ;

@Plugin(type = Command.class, menuPath = "Ulman>SimViewer",
        name = "EmbryoGen_SimViewerPlugin", headless = true,
		  description = "Fires up the de.mpicbg.ulman.simviewer, a Scenery viewer of the running EmbryoGen simulation.")
public class simviewer_FijiPlugin implements Command
{
	@Parameter
	private LogService log;

	@Override
	public void run()
	{
		runSimViewer();
	}

	public static
	void runSimViewer()
	{
		System.out.println("hello world");
	}

	public static void main(final String... args)
	{
		//start up our own ImageJ instance
		final ImageJ ij = new net.imagej.ImageJ();
		ij.ui().showUI();

		//run this class as if from GUI
		/*
		ij.command().run(plugin_CTCmeasuresICT.class, true, "gtPath",args[0], "resPath",args[1],
			"calcTRA",true, "calcSEG",true);

		//and close the IJ instance...
		//ij.appEvent().quit();
		*/

		runSimViewer();
	}
}
