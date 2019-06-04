package de.mpicbg.ulman.simviewer.aux;

import java.util.Locale;
import java.util.Scanner;

import de.mpicbg.ulman.simviewer.DisplayScene;

/**
 * A class to parse the messages according the "network-protocol" and
 * to command the SimViewer consequently. The "network-protocol" consists
 * of commands to draw, update, or delete primitive graphics such as points
 * or lines. The protocol is the best defined in the EmbryoGen simulator's
 * code, see the file DisplayUnits/SceneryDisplayUnit.cpp in there. The protocol
 * is utilized, e.g., in the CommandFromNetwork and CommandFromFlightRecorder
 * classes.
 *
 * This file was created and is being developed by Vladimir Ulman, 2019.
 */
public class NetMessagesProcessor
{
	/** constructor to store the connection to a displayed window
	    that shall be commanded from the incoming messages */
	public NetMessagesProcessor(final DisplayScene _scene)
	{
		scene = _scene;
	}

	/** the entry function to process the incoming message; since the "tick" message
	    may trigger a short waiting before a screen shot of the commanding window is
	    requested (see the code of the processTickMessage()) and the waiting can be
	    interrupted, this method may throw an InterruptedException */
	public
	void processMsg(final String msg)
	throws InterruptedException
	{
	 synchronized (scene.lockOnChangingSceneContent)
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
				System.out.println("NetMessagesProcessor: Don't understand this msg: "+msg);
		}
		catch (java.util.InputMismatchException e) {
			System.out.println("NetMessagesProcessor: Parsing error: " + e.getMessage());
		}
	 }
	}
	//----------------------------------------------------------------------------

	/** reference on the controlled rendering display */
	private final DisplayScene scene;


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
			System.out.println("NetMessagesProcessor: Don't understand this msg: "+msg);
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
			System.out.println("NetMessagesProcessor: Don't understand this msg: "+msg);
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
			System.out.println("NetMessagesProcessor: Don't understand this msg: "+msg);
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
		System.out.println("NetMessagesProcessor: not implemented yet: "+msg);
	}


	/** this is a general (free format) message, which is assumed
	    to be sent typically after one simulation round is over */
	private
	void processTickMessage(final String msg)
	throws InterruptedException
	{
		System.out.println("NetMessagesProcessor: Got tick message: "+msg);

		//check if we should save the screen
		if (scene.savingScreenshots)
		{
			//give scenery some grace time to redraw everything
			try {
				Thread.sleep(2000);
			} catch (InterruptedException e) {
				//a bit unexpected to be stopped here, so we leave a note and forward the exception upstream
				System.out.println("NetMessagesProcessor: Interrupted just before requesting a screen shot:");
				e.printStackTrace();
				throw e;
			}

			scene.saveNextScreenshot();
		}

		if (scene.garbageCollecting) scene.garbageCollect();

		scene.increaseTickCounter();
	}
}
