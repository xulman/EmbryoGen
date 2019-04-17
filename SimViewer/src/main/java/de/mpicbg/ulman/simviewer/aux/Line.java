package de.mpicbg.ulman.simviewer.aux;

import cleargl.GLVector;

/** corresponds to one element that simulator's DrawLine() can send */
public class Line
{
	public Line()                              { node = null; }
	public Line(final graphics.scenery.Line l) { node = l; }

	public final graphics.scenery.Line node;
	public final GLVector posA = new GLVector(0.f,3);
	public final GLVector posB = new GLVector(0.f,3);
	public int color;

	public int lastSeenTick = 0;

	public void update(final Line l)
	{
		posA.set(0, l.posA.x());
		posA.set(1, l.posA.y());
		posA.set(2, l.posA.z());
		posB.set(0, l.posB.x());
		posB.set(1, l.posB.y());
		posB.set(2, l.posB.z());
		color = l.color;
	}
}
