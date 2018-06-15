package de.mpicbg.ulman.simviewer;

import java.util.Scanner;

import org.zeromq.ZMQ;
import org.zeromq.ZMQException;

import de.mpicbg.ulman.simviewer.DisplayScene;
import de.mpicbg.ulman.simviewer.agents.Cell;

/**
 * Adapted from TexturedCubeJavaExample.java from the scenery project,
 * originally created by kharrington on 7/6/16.
 *
 * Current version is created by Vladimir Ulman, 2018.
 */
public class NetworkScene implements Runnable
{
	/** constructor to create connection to a displayed window */
	public NetworkScene(final DisplayScene _scene)
	{
		scene = _scene;
	}

	/** reference on the controlled rendering display */
	private final DisplayScene scene;


	/** reads the console and dispatches the commands */
	public void run()
	{
		//start receiver in an infinite loop
		System.out.println("Network listener: Started.");

		//init the communication side
		final ZMQ.Context zmqContext = ZMQ.context(1);
		ZMQ.Socket socket = null;
		try {
			socket = zmqContext.socket(ZMQ.PAIR); //NB: CLIENT/SERVER from v4.2 is not available yet
			if (socket == null)
				throw new Exception("Network listener: Cannot obtain local socket.");

			//port to listen for incoming data
			//socket.subscribe(new byte[] {});
			socket.bind("tcp://*:8765");

			//the incoming data buffer
			String msg = null;

			while (true)
			{
				msg = socket.recvStr(ZMQ.NOBLOCK);
				if (msg != null)
					processMsg(msg);
				else
					Thread.sleep(1000);
			}
		}
		catch (ZMQException e) {
			System.out.println("Network listener: Crashed with ZeroMQ error: " + e.getMessage());
		}
		catch (InterruptedException e) {
			//System.out.println("Network listener: Stopped.");
		}
		catch (Exception e) {
			System.out.println("Network listener: Error: " + e.getMessage());
		}
		finally {
			if (socket != null)
			{
				socket.unbind("tcp://*:8765");
				socket.close();
			}
			//zmqContext.close();
			//zmqContext.term();

			System.out.println("Network listener: Stopped.");
		}
	}


	private
	void processMsg(final String msg)
	{
		if (msg.startsWith("v1 points")) processPoints(msg);
		else
		if (msg.startsWith("v1 lines")) processLines(msg);
		else
		if (msg.startsWith("v1 vectors")) processVectors(msg);
		else
		if (msg.startsWith("v1 triangles")) processTriangles(msg);
		else
			System.out.println("Don't understand this msg: "+msg);
	}

	private
	void processPoints(final String msg)
	{
		Scanner s = new Scanner(msg);

		//System.out.println("processing point msg: "+msg);

		//this skips the "v1 points" - the two tokens
		s.next();
		s.next();
		final int N = s.nextInt();

		//is the next token 'dim'?
		if (s.next("dim").startsWith("dim") == false)
		{
			System.out.println("Don't understand this msg: "+msg);
			s.close();
			return;
		}

		//so the next token is dimensionality of the points
		final int D = s.nextInt();

		//now, point by point is reported
		//
		//we will be updating consequent spheres within a cell as long as we
		//will be reading the same ID again and again
		int lastID = -999999888;
		int pointCount = 0;
		Cell cell = null;

		for (int n=0; n < N; ++n)
		{
			//extract the point ID
			final int ID = s.nextInt();

			//is it a new block of points with the same ID?
			if (ID != lastID)
			{
				//yes...
				//update previously finished block (if there was some)
				if (cell != null) scene.UpdateCellSphereNodes(cell);

				//get ready for the processing of the new block
				lastID = ID;
				pointCount = 0;
				cell = scene.cellsData.get(ID);
				if (cell == null)
				{
					//hmm, this cell ID is new to me...
					cell = new Cell(4,10); //TODO, how much?? now adjusted for TRAgen
					cell.ID = ID;
					scene.cellsData.put(cell.ID,cell);
					//System.out.println("Starting ID "+ID);
				}
				//else System.out.println("Updating ID "+ID);
			}
			//no, it is just another point within the block
			else ++pointCount;

			//now read and save coordinates
			int d=0;
			for (; d < D && d < 3; ++d)
			{
				cell.sphereCentres[3*pointCount +d]=s.nextFloat();
			}
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			cell.sphereRadii[pointCount]  = s.nextFloat();
			cell.sphereColors[pointCount] = s.nextInt();
		}

		//update previously finished block (if there was some)
		if (cell != null) scene.UpdateCellSphereNodes(cell);

		s.close();
	}

	private
	void processLines(final String msg)
	{
		System.out.println("not implemented yet: "+msg);
	}

	private
	void processVectors(final String msg)
	{
		System.out.println("not implemented yet: "+msg);
	}

	private
	void processTriangles(final String msg)
	{
		System.out.println("not implemented yet: "+msg);
	}
}
