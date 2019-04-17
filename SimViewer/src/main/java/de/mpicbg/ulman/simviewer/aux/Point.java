package de.mpicbg.ulman.simviewer.aux;

import cleargl.GLVector;
import graphics.scenery.Node;

/** corresponds to one element that simulator's DrawPoint() can send */
public class Point
{
	public Point()             { node = null; }   //without connection to Scenery
	public Point(final Node n) { node = n; }      //  with  connection to Scenery

	public final Node node;
	public final GLVector centre = new GLVector(0.f,3);
	public final GLVector radius = new GLVector(0.f,3);
	public int color;

	public int lastSeenTick = 0;

	public void update(final Point p)
	{
		centre.set(0, p.centre.x());
		centre.set(1, p.centre.y());
		centre.set(2, p.centre.z());
		radius.set(0, p.radius.x());
		radius.set(1, p.radius.y());
		radius.set(2, p.radius.z());
		color = p.color;
	}
}
