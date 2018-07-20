package de.mpicbg.ulman.simviewer.agents;

import java.util.List;
import java.util.LinkedList;
import java.util.Vector;
import graphics.scenery.Node;

/**
 * Container to keep any graphics primitives that "can arrive" via
 * the class NetworkScene. Unlike Cell, which assumes certain content
 * (e.g. fixed number of spheres and force vectors) and can "refresh"
 * its content, this class is designed to add new content incrementally
 * and display/undisplay it without the ability of updating it.
 *
 * If subsequent network data arrive (with their ID being the same
 * as the ID of this container), it is always added in here. There
 * is no contract on how to identify and thus replace or delete anything
 * already contained in this container. If updating the content is desired,
 * one must always create a new container with the "updated" data.
 *
 * The class uses synchronized lists to allow Network thread to feed them
 * while the Display thread is reading them. Only the Nodes are stored
 * via the Vector because we want to sweep them fast, and they accessed
 * exclusively via the Display thread.
 *
 * Created by Vladimir Ulman, 2018.
 */
public class Container
{
	/** this container ID, it should match the key in DisplayScene.containersData map */
	public int ID;


	public final List<Point>  pointData;
	public final Vector<Node> pointNodes;
	//
	/** flag to know whether this.pointNodes are currently added/displayed in the scene */
	public boolean areAllPointsDisplayed = false;


	public final List<Point>  lineData;
	public final Vector<Node> lineNodes;
	//
	/** flag to know whether this.lineNodes are currently added/displayed in the scene */
	public boolean areLinesDisplayed = false;


	public final List<Point>  vectorData;
	public final Vector<Node> vectorNodes;
	//
	/** flag to know whether this.forceNodes are currently added/displayed in the scene */
	public boolean areVectorsDisplayed = false;


	//TODO: meshes


	public Cell()
	{
		pointData  = Collections.synchronizedList(new LinkedList<>());
		lineData   = Collections.synchronizedList(new LinkedList<>());
		vectorData = Collections.synchronizedList(new LinkedList<>());

		pointNodes  = Vector<>(50);
		lineNodes   = Vector<>(50);
		vectorNodes = Vector<>(50);
	}


	/** corresponds to one element that simulator's DrawPoint() can send */
	public class Point
	{
		float[3] centre;
		float radius;
		int color;
	}

	/** corresponds to one element that simulator's DrawLine() can send */
	public class Line
	{
		float[3] posA;
		float[3] posB;
		int color;
	}

	/** corresponds to one element that simulator's DrawVector() can send */
	public class Vector
	{
		float[3] base;
		float[3] vector;
		int color;
	}

	//TODO: corresponds to one element that simulator's DrawTriangle() can send
}
