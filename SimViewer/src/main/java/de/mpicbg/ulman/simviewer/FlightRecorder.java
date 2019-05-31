package de.mpicbg.ulman.simviewer;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.io.IOException;
import java.util.ListIterator;

/**
 * Operates on a given file that consists of messages that could be normally
 * transferred over the network but were instead saved into this file.
 * Owing to this class, the messages can be extracted from the file, and
 * can be injected to the NetworkScene as if the messages would come over
 * the network. This way, a simulation can be replayed (even backwards!)
 * provided the file's content shows a history of some simulation -- in
 * which case the individual time points are separated with the "tick"
 * messages.
 *
 * This file was created and is being developed by Vladimir Ulman, 2019.
 */
public class FlightRecorder
{
	/** constructor to create connection to a displayed window */
	public FlightRecorder(final NetworkScene ns)
	{
		//keep the reference
		netCommandsProcessor = ns;
	}

	/** setup the reading of the FRfilename  (FR = Flight Recording) */
	public synchronized
	void open(final String FRfilename)
	throws IOException
	{
		nextMsgs = null;
		//NB: stays null if the consequent operations should fail

		nextMsgs = Files.readAllLines(Paths.get(FRfilename)).listIterator();
		sendNextTimepointMessages();
	}

	/** reference on the processor of the network (display) commands */
	private
	final NetworkScene netCommandsProcessor;

	/** should always point just before the first message of a time point,
	    this time point is termed as the "next time point"; that often
	    means that this iterator should point just after some "tick" message */
	private
	ListIterator<String> nextMsgs = null;

	//--------------------------------------------

	/** extracts the messages from the current timepoint up to the next one,
	    and sends the messages to the associated NetworkScene; returns true
	    if the operation was successful */
	public synchronized
	boolean sendNextTimepointMessages()
	{
		if (nextMsgs == null) return false; //stop if the no file is opened

		//advance and "transmit" the messages up to the next tick messages
		boolean readNextMsg = true;
		while (nextMsgs.hasNext() && readNextMsg)
		{
			final String msg = nextMsgs.next();
			netCommandsProcessor.processMsg(msg);

			if (msg.startsWith("v1 tick")) readNextMsg = false;
		}

		return true;
	}

	/** extracts the messages from the previous timepoint (that is the time point
	    that is just before this one, which was just replayed), and sends the messages
	    to the associated NetworkScene; returns true if the operation was successful */
	public synchronized
	boolean sendPrevTimepointMessages()
	{
		if (nextMsgs == null) return false; //stop if the no file is opened

		//skip (very likely) over the "tick" message of the currently-replayed time point
		if (nextMsgs.hasPrevious()) nextMsgs.previous();

		//list back until "tick" message is found -- that was over the currently-replayed point
		while (nextMsgs.hasPrevious() && !nextMsgs.previous().startsWith("v1 tick"));

		//list back until "tick" message is found -- over the wanted time point,
		//and move forward over it (to satisfy the nextMsgs's "invariant")
		while (nextMsgs.hasPrevious() && !nextMsgs.previous().startsWith("v1 tick"));
		if (nextMsgs.hasNext()) nextMsgs.next();

		return sendNextTimepointMessages();
	}

	/** similar to the sendNextTimepointMessages() except that it acts on the very
	    first time point (that is recorded in the currently opened file);
	    returns true if the operation was successful */
	public synchronized
	boolean rewindAndSendFirstTimepoint()
	{
		if (nextMsgs == null) return false; //stop if the no file is opened

		//reach the very beginning and then replay the "next" time point
		while (nextMsgs.hasPrevious()) nextMsgs.previous();
		return sendNextTimepointMessages();
	}

	/** similar to the sendNextTimepointMessages() except that it acts on the very
	    last time point (that is recorded in the currently opened file);
	    returns true if the operation was successful */
	public synchronized
	boolean rewindAndSendLastTimepoint()
	{
		if (nextMsgs == null) return false; //stop if the no file is opened

		//reach the very end and then traverse back over the tp to be sent (the last tp)
		while (nextMsgs.hasNext()) nextMsgs.next();

		//skip (very likely) over the last "tick" message (if present...)
		if (nextMsgs.hasPrevious()) nextMsgs.previous();

		//list back until "tick" message is found, and move forward over it
		while (nextMsgs.hasPrevious() && !nextMsgs.previous().startsWith("v1 tick"));
		if (nextMsgs.hasNext()) nextMsgs.next();

		return sendNextTimepointMessages();
	}
}
