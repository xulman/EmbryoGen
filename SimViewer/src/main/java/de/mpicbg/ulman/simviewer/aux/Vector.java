package de.mpicbg.ulman.simviewer.aux;

import cleargl.GLVector;
import graphics.scenery.Arrow;

/** corresponds to one element that simulator's DrawVector() can send */
public class Vector
{
	public Vector()              { node = null; }
	public Vector(final Arrow v) { node = v; }

	public final Arrow node;
	public final GLVector base   = new GLVector(0.f,3);
	public final GLVector vector = new GLVector(0.f,3);
	public int color;

	public int lastSeenTick = 0;

	public void update(final Vector v)
	{
		base.set(0, v.base.x());
		base.set(1, v.base.y());
		base.set(2, v.base.z());
		vector.set(0, v.vector.x());
		vector.set(1, v.vector.y());
		vector.set(2, v.vector.z());
		color = v.color;
	}
}
