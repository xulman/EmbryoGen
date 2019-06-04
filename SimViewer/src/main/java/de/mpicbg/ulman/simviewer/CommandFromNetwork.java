package de.mpicbg.ulman.simviewer;

import org.zeromq.SocketType;
import org.zeromq.ZMQ;
import org.zeromq.ZMQException;

import de.mpicbg.ulman.simviewer.aux.NetMessagesProcessor;

/**
 * Operates on a network socket and listens for incoming messages.
 * Owing to this class, the messages can be extracted from the network, and
 * be processed with the NetMessagesProcessor to update the SimViewer's
 * displayed content. This way, an online view on the current state of
 * a running simulation can be observed provided the simulator is "broadcasting"
 * the relevant messages (including the "tick" messages to denote between
 * individual simulated time points).
 *
 * This file was created and is being developed by Vladimir Ulman, 2018.
 */
public class CommandFromNetwork implements Runnable
{
	/** constructor to create connection (listening at the 8765 port),
	    and link it to a shared NetMessagesProcessor (that connects this
	    'commander' to the displayed window */
	public CommandFromNetwork(final NetMessagesProcessor nmp)
	{
		netMsgProcessor = nmp;
		listenOnPort = 8765;
	}

	/** constructor to create connection (listening at the given port),
	    and link it to a shared NetMessagesProcessor (that connects this
	    'commander' to the displayed window */
	public CommandFromNetwork(final NetMessagesProcessor nmp, final int _port)
	{
		netMsgProcessor = nmp;
		listenOnPort = _port;
	}

	/** reference on the messages processor */
	private
	final NetMessagesProcessor netMsgProcessor;

	/** the port to listen at */
	private
	final int listenOnPort;

	//--------------------------------------------

	/** listens on the network and dispatches the commands */
	public void run()
	{
		//start receiver in an infinite loop
		System.out.println("Network listener: Started on port "+listenOnPort+".");

		//init the communication side
		final ZMQ.Context zmqContext = ZMQ.context(1);
		ZMQ.Socket socket = null;
		try {
			socket = zmqContext.socket(SocketType.PAIR);
			if (socket == null)
				throw new Exception("Network listener: Cannot obtain local socket.");

			//port to listen for incoming data
			//socket.subscribe(new byte[] {});
			socket.bind("tcp://*:"+listenOnPort);

			//the incoming data buffer
			String msg;

			while (true)
			{
				msg = socket.recvStr(ZMQ.NOBLOCK);
				if (msg != null)
					netMsgProcessor.processMsg(msg);
				else
					Thread.sleep(1000);
			}
		}
		catch (ZMQException e) {
			System.out.println("Network listener: Crashed with ZeroMQ error: " + e.getMessage());
		}
		catch (InterruptedException e) {
			System.out.println("Network listener: Interrupted.");
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
}
