package de.mpicbg.ulman.simviewer;

import de.mpicbg.ulman.simviewer.DisplayScene;

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
		//this is the main loop: the program stays here until the main window is closed
		scene.main();
	}


            /*
            Thread rotator = new Thread(){
                public void run() {
                    while (true) {
                        box.getRotation().rotateByAngleY(0.01f);
                        box.setNeedsUpdate(true);

                        try {
                            Thread.sleep(20);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
            };
            rotator.start();
            */
}

