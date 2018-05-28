package de.mpicbg.ulman.simviewer;

import java.io.IOException;
import java.util.Scanner;
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

	private final DisplayScene scene;

	/** reads the console and dispatches the commands */
	public void run()
	{
		try {
			while (Thread.interrupted() == false)
			{
				int key = System.in.read();
				switch (key)
				{
				case 'h':
					System.out.println("List of accepted commands:");
					System.out.println("h - Shows this help message");
					System.out.println("q - Quits the program");
					System.out.println("a - Toggles display of the axes in the scene centre");
					System.out.println("b - Toggles display of the scene border");
					System.out.println("c - Toggles display of the cells");
					break;

				case 'q':
					scene.stop();
					break;

				case 'a':
					scene.ToggleDisplayAxes();
					break;
				case 'b':
					scene.ToggleDisplaySceneBorder();
					break;
				case 'c':
					scene.UpdateCell();
					break;
				}
			}
		}
		catch (IOException e) {
			System.out.print("Key listener: Error reading the console.");
			e.printStackTrace();
		}

		System.out.print("Key listener: Stopped.");
	}
}
