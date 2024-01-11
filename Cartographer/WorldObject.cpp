#include "WorldObject.h"

WorldObject::WorldObject(QObject *parent)
	: m_username("N/A")
	, m_heightEstimate(1.0f)
	, m_heightConfidence(0.0f)
	, QObject(parent) {
	m_worldTrans = new FbxVector4();
	m_vertexRefs = new QVector<quint64>;
	m_verticesFootprint = new QVector<QVector2D>;
	m_verticesFootprintLocal = new QVector<QVector2D>;
	m_vertices3D = new QVector<FbxVector4>;
	//m_vertexNormals = new QVector<QVector3D>;
	m_metadata = new QMap<QString, QString>;
}

WorldObject::~WorldObject() {
	if(m_worldTrans) {
		delete m_worldTrans;
	}
	if(m_vertexRefs) {
		delete m_vertexRefs;
	}
	if(m_verticesFootprint) {
		delete m_verticesFootprint;
	}
	if(m_verticesFootprintLocal) {
		delete m_verticesFootprintLocal;
	}
	if(m_vertices3D) {
		delete m_vertices3D;
	}
	/*
	if(m_vertexNormals) {
		delete m_vertexNormals;
	}
	*/
	if(m_metadata) {
		delete m_metadata;
	}
}

void WorldObject::CalcWorldTrans() {
		int idx = 0;
		qreal avgX = 0;
		qreal avgY = 0;

		Q_FOREACH(QVector2D vertex, m_verticesFootprint->toList()) {
			avgX += vertex.x();
			avgY += vertex.y();
			idx++;
		}
		
		avgX /= idx;
		avgY /= idx;

		m_worldTrans->Set(avgX, 0, avgY);
}

void WorldObject::Localize() {
	Q_FOREACH(QVector2D vertex2d, m_verticesFootprint->toList()) {
		vertex2d -= QVector2D(m_worldTrans->mData[0], m_worldTrans->mData[2]);
		m_verticesFootprintLocal->append(vertex2d);
	}
}

void WorldObject::Extrude() {
	Q_FOREACH(QVector2D vertex2d, m_verticesFootprintLocal->toList()) {
		FbxVector4 vertex3d;

		vertex3d.Set(vertex2d.x(), 0, vertex2d.y());
		m_vertices3D->append(vertex3d);
		vertex3d.Set(vertex2d.x(), m_heightEstimate, vertex2d.y());
		m_vertices3D->append(vertex3d);
	}
}