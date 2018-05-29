package de.mpicbg.ulman.simviewer;

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
		System.out.println("got msg: "+msg);
	}
}
