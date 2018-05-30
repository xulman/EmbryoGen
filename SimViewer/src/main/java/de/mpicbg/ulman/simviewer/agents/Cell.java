package de.mpicbg.ulman.simviewer.agents;

import graphics.scenery.Node;

/**
 * All, yet minimal, information necessary to display a cell and its status.
 * This includes the cell shape and forces acting on it.
 *
 * Created by Vladimir Ulman, 2018.
 */
public class Cell
{
	//should match the key in DisplayScene.cellsData map
	public int ID;

	public float[] sphereRadii;
	public float[] sphereCentres; //length must be 3*sphereRadii.length
	public int[]   sphereColors;
	//
	public final Node[] sphereNodes;
	//regardless of the actual value of the radius:
	//once Cell exists, all elements of its ...Nodes[] must not be null)
	//NB: if sphereRadii[i] == 0, then sphereNodes[i] is scaled to a point and not displayed


	public int[]   forceColors;
	public float[] forceVectors; //length must be 3*forceColors.length
	//
	public final Node[] forceNodes;

	public Cell(final int sphereCapacity, final int vectorCapacity)
	{
		sphereRadii   = new float[  sphereCapacity];
		sphereCentres = new float[3*sphereCapacity];
		sphereColors  = new   int[  sphereCapacity];
		sphereNodes   = new  Node[  sphereCapacity];

		forceColors  = new   int[  vectorCapacity];
		forceVectors = new float[3*vectorCapacity];
		forceNodes   = new  Node[  vectorCapacity];
	}
}
