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
			System.out.println("a - Toggles display of the axes in the scene centre");
			System.out.println("b - Toggles display of the scene border");
			System.out.println("c - Toggles display of the cells");
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

		case 'q':
			scene.stop();
			//don't wait for GUI to tell me to stop
			throw new InterruptedException("Stop myself now.");
		}
	}
}
