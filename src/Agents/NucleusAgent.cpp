#include "NucleusAgent.hpp"
#include "../util/surfacesamplers.hpp"
#include <fmt/core.h>

void NucleusAgent::adjustGeometryByForces(void) {
	// TRAgen paper, eq (1):
	// reset the array with final forces (which will become accelerations soon)
	for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i)
		accels[i] = 0;

	// collect all forces acting on every sphere to have one overall force per
	// sphere
	for (const auto& f : forces)
		accels[f.hint] += f;

#ifndef NDEBUG
	if (detailedReportingMode) {
		for (const auto& f : forces)
			report::message(
			    fmt::format("{}: ||= {} \tforce {}", ID, f.len(), toString(f)));

		std::ostringstream forcesReport;
		forcesReport << ID << ": final forces";
		for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i)
			forcesReport << ", |" << i << "|=" << accels[i].len();
		report::message(forcesReport.str());
	}
#endif
	// now, translation is a result of forces:
	for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i) {
		// calculate accelerations: F=ma -> a=F/m
		// TODO: volume of a sphere should be taken into account
		accels[i] /= weights[i];

		// velocities: v=at
		velocities[i] += incrTime * accels[i];

		// displacement: |trajectory|=vt
		futureGeometry.centres[i] += incrTime * velocities[i];
	}

	// update AABB to the new geometry
	futureGeometry.Geometry::updateOwnAABB();

	// all forces processed...
	forces.clear();
}

void NucleusAgent::advanceAndBuildIntForces(const float futureGlobalTime) {
	// call the "texture hook"!
	advanceAgent(futureGlobalTime);

	// add forces on the list that represent how and where the nucleus would
	// like to move TRAgen paper, eq (2): Fdesired = weight *
	// drivingForceMagnitude NB: the forces will act rigidly on the full nucleus
	for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i) {
		forces.emplace_back((weights[i] / velocity_PersistenceTime) *
		                        velocity_CurrentlyDesired,
		                    futureGeometry.centres[i], i, ftype_drive);
	}

#ifndef NDEBUG
	// export forces for display:
	forcesForDisplay = forces;
#endif
	// increase the local time of the agent
	currTime += incrTime;
}

void NucleusAgent::collectExtForces() {
	// damping force (aka friction due to the environment,
	// an ext. force that is independent of other agents)
	// TRAgen paper, eq. (3)
	for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i) {
		forces.emplace_back((-weights[i] / velocity_PersistenceTime) *
		                        velocities[i],
		                    futureGeometry.centres[i], i, ftype_friction);
	}

	// scheduler, please give me ShadowAgents that are not further than
	// ignoreDistance (and the distance is evaluated based on distances of
	// AABBs)
	std::vector<const NamedAxisAlignedBoundingBox*> nearbyAgentBoxes;
	Officer->getNearbyAABBs(this, ignoreDistance, nearbyAgentBoxes);

#ifndef NDEBUG
	if (detailedReportingMode)
		report::message(fmt::format("ID {}: Found {} nearby agents", ID,
		                            nearbyAgentBoxes.size()));
#endif
	// those on the list are ShadowAgents who are potentially close enough
	// to interact with me and these I need to inspect closely
	proximityPairs_toNuclei.clear();
	proximityPairs_toYolk.clear();
	proximityPairs_tracks.clear();
	for (const auto naabb : nearbyAgentBoxes) {
		const std::string& agentType =
		    Officer->translateNameIdToAgentName(naabb->nameID);
		// fetch the relevant ShadowAgent only now -- when we really know
		// that we want this one
		const ShadowAgent* sa = Officer->getNearbyAgent(naabb->ID);
		if (agentType[0] == 'n') {
			report::debugError(
			    fmt::format("XXFO#{} Before get distance", Officer->getID()));
			geometry->getDistance(sa->getGeometry(), proximityPairs_toNuclei,
			                      const_cast<ShadowAgent*>(sa));
			report::debugError(
			    fmt::format("XXFO#{} After get distance", Officer->getID()));
		} else {
			if (agentType[0] == 'y') {
				geometry->getDistance(sa->getGeometry(), proximityPairs_toYolk);
			} else {
				geometry->getDistance(sa->getGeometry(), proximityPairs_tracks);
			}
		}
	}

	if (detailedReportingMode) {
		report::debugMessage(
		    fmt::format("ID {}: Found {} proximity pairs to nuclei", ID,
		                proximityPairs_toNuclei.size()));
		report::debugMessage(
		    fmt::format("ID {}: Found {} proximity pairs to yolk", ID,
		                proximityPairs_toYolk.size()));
		report::debugMessage(fmt::format(
		    "ID {}: Found {}  proximity pairs with guiding trajectories", ID,
		    proximityPairs_tracks.size()));
	}

	// now, postprocess the proximityPairs, that is, to
	// convert proximityPairs_toNuclei to forces according to TRAgen rules
	Vector3d<float> f, g; // tmp vectors
	for (const auto& pp : proximityPairs_toNuclei) {
		if (pp.distance > 0) {
			if (detailedReportingMode)
				report::debugMessage(fmt::format(
				    "{}: repulsive  pp.distance={}", ID, pp.distance));

			// no collision
			if (pp.distance < 3.0) // TODO: replace 3.0 with some function of
			                       // fstrength_rep_scale
			{
				// distance not too far, repulsion makes sense here
				//
				// unit force vector (in the direction "away from the other
				// buddy")
				f = pp.localPos;
				f -= pp.otherPos;
				f.changeToUnitOrZero();

				// TRAgen paper, eq. (4)
				forces.emplace_back(
				    (fstrength_overlap_level *
				     std::exp(-pp.distance / fstrength_rep_scale)) *
				        f,
				    futureGeometry.centres[pp.localHint], pp.localHint,
				    ftype_repulsive);
			}
		} else {
			// collision, pp.distance <= 0
			// NB: in collision, the other surface is within local volume, so
			//     the vector local->other actually points in the opposite
			//     direction! (as local surface further away than other surface
			//     from local centre)

			// body force
			//
			// unit force vector (in the direction "away from the other buddy")
			f = pp.otherPos;
			f -= pp.localPos;
			f.changeToUnitOrZero();

			float fScale = fstrength_overlap_level;
			if (-pp.distance > fstrength_overlap_depth) {
				// in the non-calm response zone (where force increases with the
				// penetration depth)
				fScale += fstrength_overlap_scale *
				          float(-pp.distance - fstrength_overlap_depth);
			}

			// TRAgen paper, eq. (5)
			forces.emplace_back(fScale * f,
			                    futureGeometry.centres[pp.localHint],
			                    pp.localHint, ftype_body);

			if (detailedReportingMode)
				report::debugMessage(
				    fmt::format("{}: body  pp.distance={} |force|={}", ID,
				                pp.distance, fScale * f.len()));

			// sliding force
			//
			// difference of velocities
			g = static_cast<const NucleusAgent*>(pp.callerHint)
			        ->getVelocityOfSphere(pp.otherHint);
			g -= velocities[pp.localHint];

			if (detailedReportingMode)
				report::debugMessage(fmt::format(
				    "{}: slide oID={}  |velocityDiff|={}", ID,
				    ((const NucleusAgent*)pp.callerHint)->ID, g.len()));

			// subtract from it the component that is parallel to this proximity
			// pair
			f *= dotProduct(f, g); // f is now the projection of g onto f
			g -= f; // g is now the difference of velocities without the
			        // component that is parallel with the proximity pair

			// TRAgen paper, somewhat eq. (6)
			g *= fstrength_slide_scale * weights[pp.localHint] /
			     velocity_PersistenceTime;
			// "surface friction coeff" | velocity->force, the same as for
			// ftype_drive
			forces.emplace_back(g, futureGeometry.centres[pp.localHint],
			                    pp.localHint, ftype_slide);
#ifndef NDEBUG
			Officer->reportOverlap(-float(pp.distance));
#endif
		}
	}

	// non-TRAgen new force, driven by the offset distance from the position
	// expected by the shape hinter, it converts proximityPairs_toYolk to forces
	for (const auto& pp : proximityPairs_toYolk)
		if (pp.localHint ==
		    0) // consider pair related only to the first sphere of a nucleus
		{
			// unit force vector (in the direction "towards the shape hinter")
			f = pp.otherPos;
			f -= pp.localPos;
			f.changeToUnitOrZero();

			if (detailedReportingMode)
				report::debugMessage(
				    fmt::format("{}: hinter pp.distance={}", ID, pp.distance));

			// the get-back-to-hinter force
			f *= 2 * fstrength_overlap_level *
			     std::min(pp.distance * pp.distance * fstrength_hinter_scale,
			              ProximityPair::precision_t(1.0));

			// apply the same force to all spheres
			for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i)
				forces.emplace_back(f, futureGeometry.centres[i], i,
				                    ftype_hinter);
		}

#ifndef NDEBUG
	// append forces to forcesForDisplay, make a copy (push_back, not
	// emplace_back)!
	for (const auto& f : forces)
		forcesForDisplay.push_back(f);
#endif
}

void NucleusAgent::drawMask(DisplayUnit& du) {
	const int color = 2;

	// if not selected: draw cells with no debug bit
	// if     selected: draw cells as a global debug object
	int dID = DisplayUnit::firstIdForAgentObjects(ID);
	int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID * 40 + 5000;
	// NB: 'd'ID = is for 'd'rawing, not for 'd'ebug !

	// draw spheres
	for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i) {
		du.DrawPoint(detailedDrawingMode ? gdID : dID,
		             futureGeometry.centres[i], float(futureGeometry.radii[i]),
		             color);
		++dID;
		++gdID; // just update both counters
	}

	// velocities -- global debug
	// for (int i=0; i < futureGeometry.noOfSpheres; ++i)
	{
		int i = 0;
		du.DrawVector(gdID++, futureGeometry.centres[i], velocities[i],
		              0); // white color
	}

	// red lines with overlapping proximity pairs to nuclei
	//(if detailedDrawingMode is true, these lines will be drawn later as "local
	// debug")
	if (!detailedDrawingMode) {
		for (const auto& p : proximityPairs_toNuclei)
			if (p.distance < 0)
				du.DrawLine(gdID++, p.localPos, p.otherPos, 1);
	}
}

void NucleusAgent::drawForDebug(DisplayUnit& du) {
	// render only if under inspection
	if (detailedDrawingMode) {
		const int color = 2;
		int dID = DisplayUnit::firstIdForAgentDebugObjects(ID);

		// cell centres connection "line" (green):
		for (std::size_t i = 1; i < futureGeometry.getNoOfSpheres(); ++i)
			du.DrawLine(dID++, futureGeometry.centres[i - 1],
			            futureGeometry.centres[i], color);

		// draw agent's periphery (as blue spheres)
		// NB: showing the cell outline, that is now updated from the
		// futureGeometry, and stored already in the geometryAlias
		SphereSampler<float> ss;
		Vector3d<float> periPoint;
		int periPointCnt = 0;

		for (std::size_t S = 0; S < geometryAlias.getNoOfSpheres(); ++S) {
			ss.resetByStepSize(float(geometryAlias.radii[S]), 2.6f);
			while (ss.next(periPoint)) {
				periPoint += geometryAlias.centres[S];

				// draw the periPoint only if it collides with no (and excluding
				// this) sphere
				if (geometryAlias.collideWithPoint(periPoint, int(S)) == -1) {
					++periPointCnt;
					du.DrawPoint(dID++, periPoint, 0.3f, 3);
				}
			}
		}
		report::debugMessage(fmt::format("{} surface consists of {} spheres",
		                                 getSignature(), periPointCnt));

		// red lines with overlapping proximity pairs to nuclei
		for (const auto& p : proximityPairs_toNuclei)
			if (p.distance < 0)
				du.DrawLine(dID++, p.localPos, p.otherPos, 1);

		// neighbors:
		// white line for the most inner spheres, yellow for second most inner
		// both showing proximity pairs to yolk (shape hinter)
		for (const auto& p : proximityPairs_toYolk)
			if (p.localHint < 2)
				du.DrawLine(dID++, p.localPos, p.otherPos,
				            (int)(p.localHint * 6));

		// magenta lines with trajectory guiding vectors
		for (const auto& p : proximityPairs_tracks)
			if (p.distance > 0)
				du.DrawVector(dID++, p.localPos, p.otherPos - p.localPos, 5);

#ifndef NDEBUG
		// forces:
		for (const auto& f : forcesForDisplay) {
			int color = 2; // default color: green (for shape hinter)
			// if      (f.type == ftype_s2s)      color = 4; //cyan
			// else if (f.type == ftype_drive)    color = 5; //magenta
			// else if (f.type == ftype_friction) color = 6; //yellow
			if (f.type == ftype_body)
				color = 4; // cyan
			else if (f.type == ftype_repulsive || f.type == ftype_drive)
				color = 5; // magenta
			else if (f.type == ftype_slide)
				color = 6; // yellow
			else if (f.type == ftype_friction)
				color = 3; // blue
			else if (f.type != ftype_hinter)
				color = -1; // don't draw
			if (color > 0)
				du.DrawVector(dID++, f.base, f, color);
		}
#endif
		// velocities:
		std::ostringstream velocitiesReport;
		//
		// choose 2nd sphere if avail, else 1st sphere if avail, else none
		// (then: Idx == -1)
		const int velocityReportIdx =
		    std::min(2, int(futureGeometry.getNoOfSpheres())) - 1;
		if (velocityReportIdx == -1)
			velocitiesReport << ID << ": no spheres -> no velocities";
		else
			velocitiesReport << ID << ": velocity[" << velocityReportIdx
			                 << "]=" << velocities[velocityReportIdx];
		//
		for (std::size_t i = 0; i < futureGeometry.getNoOfSpheres(); ++i)
			velocitiesReport << ", |" << i << "|=" << velocities[i].len();
		report::message(velocitiesReport.str());
	}
}

void NucleusAgent::drawMask(i3d::Image3d<i3d::GRAY16>& img) {
	futureGeometry.renderIntoMask(img, (i3d::GRAY16)ID);
}
