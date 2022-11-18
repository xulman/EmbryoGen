#include "GrpcDisplayUnit.hpp"

#include <memory>
#include "../util/report.hpp"


void GrpcDisplayUnit::DrawPoint(const int ID,
                                const Vector3d<float>& pos,
                                const float radius,
                                const int color) /* override */
{
	//choose which batch to use: normal vs. debug
	int origID;
	perAgentBatch_dataType& batch = shouldShowAsDebug(ID, &origID) ?
		getCurrentDebugBatchForId(origID) : getCurrentNormalBatchForId(origID);

	proto::SphereParameters* s = batch.add_spheres();
	proto::Vector3D* centre = s->mutable_centre();
	centre->set_x(pos.x);
	centre->set_y(pos.y);
	centre->set_z(pos.z);
	s->set_time(uint32_t(currentTimepoint));
	s->set_radius(radius);
	s->set_colorxrgb(uint32_t(color));
}

void GrpcDisplayUnit::DrawLine(const int ID,
                               const Vector3d<float>& posA,
                               const Vector3d<float>& posB,
                               const int color) /* override */
{
	//choose which batch to use: normal vs. debug
	int origID;
	perAgentBatch_dataType& batch = shouldShowAsDebug(ID, &origID) ?
		getCurrentDebugBatchForId(origID) : getCurrentNormalBatchForId(origID);

	proto::LineParameters* l = batch.add_lines();
	proto::Vector3D* pos = l->mutable_startpos();
	pos->set_x(posA.x);
	pos->set_y(posA.y);
	pos->set_z(posA.z);
	pos = l->mutable_endpos();
	pos->set_x(posB.x);
	pos->set_y(posB.y);
	pos->set_z(posB.z);
	l->set_time(uint32_t(currentTimepoint));
	l->set_radius(DEFAULT_LINE_RADIUS);
	l->set_colorxrgb(uint32_t(color));
}

void GrpcDisplayUnit::DrawVector(const int ID,
                                 const Vector3d<float>& pos,
                                 const Vector3d<float>& vector,
                                 const int color) /* override */
{
	//choose which batch to use: normal vs. debug
	int origID;
	perAgentBatch_dataType& batch = shouldShowAsDebug(ID, &origID) ?
		getCurrentDebugBatchForId(origID) : getCurrentNormalBatchForId(origID);

	proto::VectorParameters* l = batch.add_vectors();
	proto::Vector3D* p = l->mutable_startpos();
	p->set_x(pos.x);
	p->set_y(pos.y);
	p->set_z(pos.z);
	p = l->mutable_endpos();
	p->set_x(pos.x+vector.x);
	p->set_y(pos.y+vector.y);
	p->set_z(pos.z+vector.z);
	l->set_time(uint32_t(currentTimepoint));
	l->set_radius(DEFAULT_LINE_RADIUS);
	l->set_colorxrgb(uint32_t(color));
}

void GrpcDisplayUnit::DrawTriangle(const int /* ID */,
                                   const Vector3d<float>& /* posA */,
                                   const Vector3d<float>& /* posB */,
                                   const Vector3d<float>& /* posC */,
                                   const int /* color = 0 */) /* override */ {}

void GrpcDisplayUnit::Tick(const std::string& tick_msg) /* override */
{
	sendCurrentBatches();
	resetCurrentBatches();
	if (doTickIncrementsCurrentTimepoint) ++currentTimepoint;

	//send the message itself
	auto msg = proto::SignedTextMessage();
	msg.mutable_clientid()->set_clientname(clientName);
	msg.mutable_clientmessage()->set_msg( tick_msg );
	grpc::ClientContext ctx;
	proto::Empty empty;
	_stub->showMessage(&ctx, msg, &empty);
}


void GrpcDisplayUnit::resetCurrentBatches()
{
	report::message(fmt::format("resetting in {}", (long)this));
	currentBatches_normalContent.clear();
	currentBatches_debugContent.clear();
}


perAgentBatch_dataType& GrpcDisplayUnit::getOrCreateBatchForId(
         const int agent_id,
         std::map<int, perAgentBatch_dataType>& map,
         const std::string& designationPrefix)
{
	try { return map.at(agent_id); }
	catch (std::out_of_range&) {
		map[agent_id] = perAgentBatch_dataType();
		perAgentBatch_dataType& b = map[agent_id];
		b.mutable_clientid()->set_clientname(clientName);
		b.set_collectionname(collectionName);
		b.set_dataname(fmt::format("{} {}", designationPrefix, agent_id));
		b.set_dataid(uint64_t(agent_id));
		return b;
	}
}

perAgentBatch_dataType& GrpcDisplayUnit::getCurrentNormalBatchForId(const int agent_id)
{
	return getOrCreateBatchForId(agent_id,
		currentBatches_normalContent, normalContentDataNamePrefix);
}

perAgentBatch_dataType& GrpcDisplayUnit::getCurrentDebugBatchForId(const int agent_id)
{
	return getOrCreateBatchForId(agent_id,
		currentBatches_debugContent, debugContentDataNamePrefix);
}


void GrpcDisplayUnit::sendCurrentBatches()
{
	grpc::ClientContext ctx;
	proto::Empty empty;
	std::unique_ptr< grpc::ClientWriter<proto::BatchOfGraphics> > writer(
		doAppendInsteadOfReplace
			? _stub->addGraphics(&ctx, &empty)
			: _stub->replaceGraphics(&ctx, &empty)
	);

	report::debugMessage(fmt::format("sending {} normal and {} debug batches",
					  currentBatches_normalContent.size(), currentBatches_debugContent.size()));
	for (const auto& pair : currentBatches_normalContent) {
		if (!writer->Write( pair.second )) {
			report::error("GRPC failed writing a batch of normal graphics");
		}
	}
	for (const auto& pair : currentBatches_debugContent) {
		if (!writer->Write( pair.second )) {
			report::error("GRPC failed writing a batch of debug graphics");
		}
	}

	writer->WritesDone();
	grpc::Status status = writer->Finish();
	if (!status.ok()) {
		report::error(fmt::format(
		    "GRPC failed with code: {}\nMessage: {}\nDetails: {}",
		    status.error_code(), status.error_message(), status.error_details()));
	}
}


GrpcDisplayUnit::GrpcDisplayUnit(const std::string& clientName)
{
	report::message(fmt::format("constructing GRPC at {}", (long)this));
	std::string serverURL = fmt::format("{}:{}", config::grpc_serverAddress, config::grpc_port);
	_stub = proto::ClientToServer::NewStub(
	    grpc::CreateChannel(serverURL, grpc::InsecureChannelCredentials()) );

	setClientName(clientName);
}

void GrpcDisplayUnit::introduceClientToTheServer()
{
	proto::ClientHello helloMyNameIsMsg;
	helloMyNameIsMsg.mutable_clientid()->set_clientname(clientName);
	helloMyNameIsMsg.set_returnurl(""); //means no feedback
	grpc::ClientContext ctx;
	proto::Empty empty;
	_stub->introduceClient(&ctx, helloMyNameIsMsg, &empty);
}

GrpcDisplayUnit::~GrpcDisplayUnit() /* override */
{
	report::message(fmt::format("destructing GRPC at {}", (long)this));
	sendCurrentBatches();
}
