#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H

#include <QtWidgets/QWidget>
#include <QVector2D>
#include <QVector3D>
#include <QMap>
#include <QDebug>
#include <fbxsdk.h>

class WorldObject : public QObject
{
	Q_OBJECT

public:
	WorldObject(QObject *parent = 0);
	~WorldObject();
	
	quint64 GetId() const { return m_id; }
	void SetId(quint64 inId) { m_id = inId; }

	QString GetUsername() const { return m_username; }
	void SetUsername(QString inUname) { m_username = inUname; }

	FbxVector4* WorldTrans() const { return m_worldTrans; }
	QVector<quint64>* VertexRefs() const { return m_vertexRefs; }
	QVector<QVector2D>* VerticesFootprint() const { return m_verticesFootprint; }
	QVector<QVector2D>* VerticesFootprintLocal() const { return m_verticesFootprintLocal; }
	QVector<FbxVector4>* Vertices3D() const { return m_vertices3D; }
	//QVector<QVector3D>* VertexNormals() const { return m_vertexNormals; }
	QMap<QString, QString>* Metadata() const { return m_metadata; }
	
	void CalcWorldTrans();
	void Localize();
	void Extrude();

private:
	quint64 m_id;
	QString m_username;
	qreal m_heightEstimate;
	qreal m_heightConfidence;
	FbxVector4* m_worldTrans;
	QVector<quint64>* m_vertexRefs;
	QVector<QVector2D>* m_verticesFootprint;
	QVector<QVector2D>* m_verticesFootprintLocal;
	QVector<FbxVector4>* m_vertices3D;
	//QVector<QVector3D>* m_vertexNormals;
	QMap<QString, QString>* m_metadata;
};

#endif // WORLDOBJECT_H