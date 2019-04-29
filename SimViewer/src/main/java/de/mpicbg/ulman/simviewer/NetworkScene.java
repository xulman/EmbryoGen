package de.mpicbg.ulman.simviewer;

import java.util.Locale;
import java.util.Scanner;

import org.zeromq.SocketType;
import org.zeromq.ZMQ;
import org.zeromq.ZMQException;

import de.mpicbg.ulman.simviewer.aux.Point;
import de.mpicbg.ulman.simviewer.aux.Line;
import de.mpicbg.ulman.simviewer.aux.Vector;

/**
 * Adapted from TexturedCubeJavaExample.java from the scenery project,
 * originally created by kharrington on 7/6/16.
 *
 * This file was created and is being developed by Vladimir Ulman, 2018.
 */
public class NetworkScene implements Runnable
{
	final int listenOnPort;

	/** constructor to create connection (listening at the 8765 port) to a displayed window */
	public NetworkScene(final DisplayScene _scene)
	{
		scene = _scene;
		listenOnPort = 8765;
	}

	/** constructor to create connection (listening at the given port) to a displayed window */
	public NetworkScene(final DisplayScene _scene, final int _port)
	{
		scene = _scene;
		listenOnPort = _port;
	}

	/** reference on the controlled rendering display */
	private final DisplayScene scene;


	/** reads the console and dispatches the commands */
	public void run()
	{
		//start receiver in an infinite loop
		System.out.println("Network listener: Started on port "+listenOnPort+".");

		//init the communication side
		final ZMQ.Context zmqContext = ZMQ.context(1);
		ZMQ.Socket socket = null;
		try {
			//socket = zmqContext.socket(ZMQ.PAIR); //NB: CLIENT/SERVER from v4.2 is not available yet
			socket = zmqContext.socket(SocketType.PAIR);
			if (socket == null)
				throw new Exception("Network listener: Cannot obtain local socket.");

			//port to listen for incoming data
			//socket.subscribe(new byte[] {});
			socket.bind("tcp://*:"+listenOnPort);

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
			e.printStackTrace();
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
		try {
			if (msg.startsWith("v1 points")) processPoints(msg);
			else
			if (msg.startsWith("v1 lines")) processLines(msg);
			else
			if (msg.startsWith("v1 vectors")) processVectors(msg);
			else
			if (msg.startsWith("v1 triangles")) processTriangles(msg);
			else
			if (msg.startsWith("v1 tick")) processTickMessage(msg.substring(8));
			else
				System.out.println("Don't understand this msg: "+msg);
		}
		catch (java.util.InputMismatchException e) {
			System.out.println("Parsing error: " + e.getMessage());
		}
	}

	private
	void processPoints(final String msg)
	{
		Scanner s = new Scanner(msg).useLocale(Locale.ENGLISH);

		//System.out.println("processing point msg: "+msg);

		//this skips the "v1 points" - the two tokens
		s.next();
		s.next();
		final int N = s.nextInt();

		if (N > 10) scene.suspendNodesUpdating();

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
		final Point p = new Point();
		int ID = 0;

		for (int n=0; n < N; ++n)
		{
			//extract the point ID
			ID = s.nextInt();

			//now read and save coordinates
			int d=0;
			for (; d < D && d < 3; ++d) p.centre.set(d, s.nextFloat());
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();
			//NB: all points in the same message (in this function call) are of the same dimensionality

			p.radius.set(0, s.nextFloat());
			p.radius.set(1, p.radius.x());
			p.radius.set(2, p.radius.x());
			p.color  = s.nextInt();

			scene.addUpdateOrRemovePoint(ID,p);
		}

		s.close();

		if (N > 10) scene.resumeNodesUpdating();
	}

	private
	void processLines(final String msg)
	{
		Scanner s = new Scanner(msg).useLocale(Locale.ENGLISH);

		//System.out.println("processing point msg: "+msg);

		//this skips the "v1 lines" - the two tokens
		s.next();
		s.next();
		final int N = s.nextInt();

		if (N > 10) scene.suspendNodesUpdating();

		//is the next token 'dim'?
		if (s.next("dim").startsWith("dim") == false)
		{
			System.out.println("Don't understand this msg: "+msg);
			s.close();
			return;
		}

		//so the next token is dimensionality of the points
		final int D = s.nextInt();

		//now, point pair by pair is reported
		final Line l = new Line();
		int ID = 0;

		for (int n=0; n < N; ++n)
		{
			//extract the point ID
			ID = s.nextInt();

			//now read the first in the pair and save coordinates
			int d=0;
			for (; d < D && d < 3; ++d) l.posA.set(d, s.nextFloat());
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			//now read the second in the pair and save sizes
			d=0;
			for (; d < D && d < 3; ++d) l.posB.set(d, s.nextFloat());
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			l.color = s.nextInt();

			scene.addUpdateOrRemoveLine(ID,l);
		}

		s.close();

		if (N > 10) scene.resumeNodesUpdating();
	}

	private
	void processVectors(final String msg)
	{
		Scanner s = new Scanner(msg).useLocale(Locale.ENGLISH);

		//System.out.println("processing point msg: "+msg);

		//this skips the "v1 vectors" - the two tokens
		s.next();
		s.next();
		final int N = s.nextInt();

		if (N > 10) scene.suspendNodesUpdating();

		//is the next token 'dim'?
		if (s.next("dim").startsWith("dim") == false)
		{
			System.out.println("Don't understand this msg: "+msg);
			s.close();
			return;
		}

		//so the next token is dimensionality of the points
		final int D = s.nextInt();

		//now, point pair by pair is reported
		final Vector v = new Vector();
		int ID = 0;

		for (int n=0; n < N; ++n)
		{
			//extract the point ID
			ID = s.nextInt();

			//now read the first in the pair and save coordinates
			int d=0;
			for (; d < D && d < 3; ++d) v.base.set(d, s.nextFloat());
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			//now read the second in the pair and save sizes
			d=0;
			for (; d < D && d < 3; ++d) v.vector.set(d, s.nextFloat());
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			v.color = s.nextInt();

			scene.addUpdateOrRemoveVector(ID,v);
		}

		s.close();

		if (N > 10) scene.resumeNodesUpdating();
	}

	private
	void processTriangles(final String msg)
	{
		System.out.println("not implemented yet: "+msg);
	}

	/** this is a general (free format) message, which is assumed
	    to be sent typically after one simulation round is over */
	private
	void processTickMessage(final String msg)
	{
		System.out.println("Got tick message: "+msg);

		//check if we should save the screen
		if (scene.savingScreenshots)
		{
			//give scenery some grace time to redraw everything
			try {
				Thread.sleep(2000);
			} catch (InterruptedException e) {
				e.printStackTrace();
				System.out.println("But continuing with the processing....");
			}

			scene.saveNextScreenshot();
		}

		if (scene.garbageCollecting) scene.garbageCollect();

		scene.increaseTickCounter();
	}
}
