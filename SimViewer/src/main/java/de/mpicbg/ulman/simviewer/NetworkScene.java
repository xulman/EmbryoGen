package de.mpicbg.ulman.simviewer;

import java.util.Scanner;

import org.zeromq.ZMQ;
import org.zeromq.ZMQException;

import de.mpicbg.ulman.simviewer.DisplayScene;

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
		final DisplayScene.myPoint p = scene.new myPoint();
		int ID = 0;

		for (int n=0; n < N; ++n)
		{
			//extract the point ID
			ID = s.nextInt();

			//now read and save coordinates
			int d=0;
			for (; d < D && d < 3; ++d) p.centre[d] = s.nextFloat();
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();
			//NB: all points in the same message (in this function call) are of the same dimensionality

			p.radius = s.nextFloat();
			p.color  = s.nextInt();

			scene.addUpdateOrRemovePoint(ID,p);
		}

		s.close();
	}

	private
	void processLines(final String msg)
	{
		Scanner s = new Scanner(msg);

		//System.out.println("processing point msg: "+msg);

		//this skips the "v1 lines" - the two tokens
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

		//now, point pair by pair is reported
		final DisplayScene.myLine l = scene.new myLine();
		int ID = 0;

		for (int n=0; n < N; ++n)
		{
			//extract the point ID
			ID = s.nextInt();

			//now read the first in the pair and save coordinates
			int d=0;
			for (; d < D && d < 3; ++d) l.posA[d] = s.nextFloat();
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			//now read the second in the pair and save sizes
			d=0;
			for (; d < D && d < 3; ++d) l.posB[d] = s.nextFloat();
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			l.color = s.nextInt();

			scene.addUpdateOrRemoveLine(ID,l);
		}

		s.close();
	}

	private
	void processVectors(final String msg)
	{
		Scanner s = new Scanner(msg);

		//System.out.println("processing point msg: "+msg);

		//this skips the "v1 vectors" - the two tokens
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

		//now, point pair by pair is reported
		final DisplayScene.myVector v = scene.new myVector();
		int ID = 0;

		for (int n=0; n < N; ++n)
		{
			//extract the point ID
			ID = s.nextInt();

			//now read the first in the pair and save coordinates
			int d=0;
			for (; d < D && d < 3; ++d) v.base[d] = s.nextFloat();
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			//now read the second in the pair and save sizes
			d=0;
			for (; d < D && d < 3; ++d) v.vector[d] = s.nextFloat();
			//read possibly remaining coordinates (for which we have no room to store them)
			for (; d < D; ++d) s.nextFloat();

			v.color = s.nextInt();

			scene.addUpdateOrRemoveVector(ID,v);
		}

		s.close();
	}

	private
	void processTriangles(final String msg)
	{
		System.out.println("not implemented yet: "+msg);
	}
}
