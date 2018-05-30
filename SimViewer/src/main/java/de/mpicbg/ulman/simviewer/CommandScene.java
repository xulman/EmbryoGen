package de.mpicbg.ulman.simviewer;

import java.io.InputStreamReader;
import java.io.IOException;

import de.mpicbg.ulman.simviewer.DisplayScene;
import de.mpicbg.ulman.simviewer.agents.Cell;

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
			System.out.println("d - Deletes all cells (even if not displayed)");
			System.out.println("r,R - decreases/increases radius of all cells by 0.2");
			System.out.println("x,X - moves left/right x-position of all cells by 0.2");
			break;

		case 'a':
			scene.ToggleDisplayAxes();
			break;
		case 'b':
			scene.ToggleDisplaySceneBorder();
			break;
		case 'c':
			scene.ToggleDisplayCells();
			break;
		case 'd':
			scene.RemoveCells();
			break;

		case 'r':
			for (Cell c : scene.cellsData.values())
			{
				for (int i=0; i < c.sphereRadii.length; ++i)
					c.sphereRadii[i] -= 0.2f;
				scene.UpdateCellNodes(c);
			}
			break;
		case 'R':
			for (Cell c : scene.cellsData.values())
			{
				for (int i=0; i < c.sphereRadii.length; ++i)
					c.sphereRadii[i] += 0.2f;
				scene.UpdateCellNodes(c);
			}
			break;

		case 'x':
			for (Cell c : scene.cellsData.values())
			{
				for (int i=0; i < c.sphereRadii.length; ++i)
					c.sphereCentres[3*i +0] -= 0.2f;
				scene.UpdateCellNodes(c);
			}
			break;
		case 'X':
			for (Cell c : scene.cellsData.values())
			{
				for (int i=0; i < c.sphereRadii.length; ++i)
					c.sphereCentres[3*i +0] += 0.2f;
				scene.UpdateCellNodes(c);
			}
			break;

		case 'q':
			scene.stop();
			//don't wait for GUI to tell me to stop
			throw new InterruptedException("Stop myself now.");
		}
	}
}
