#ifndef CARTOGRAPHERPROCESSOR_H
#define CARTOGRAPHERPROCESSOR_H

#include <QtWidgets/QWidget>
#include <QtNetwork>
#include <QVector2D>
#include <fbxsdk.h>

#include "WorldObject.h"

class CartographerProcessor : public QObject
{
	Q_OBJECT

public:
	CartographerProcessor();
	~CartographerProcessor();

private:
	QNetworkAccessManager* m_networkManager;
	QXmlStreamReader* m_xmlReader;
	FbxVector4 m_cameraTarget;
	QMap<quint64, QVector2D> m_nodes;
	QVector<WorldObject*> m_worldObjects;

	void parseXML(QIODevice* xml);
	void parseOSM();
	void parseBounds();
	void parseNode();
	void parseWay();
	void parseNd(WorldObject* object);
	void parseTag();
	void parseTag(WorldObject* object);
	
	void composeScene();
	bool createScene();
	void createWorldObject(int idx, WorldObject* object);
	FbxNode* createObjectMesh(FbxScene* pScene, char* pName, WorldObject* object);

signals:
	void signalProgressUpdate(QString message, int progress);
	void finished();

private slots:
	void setup();
	void slotBeginDownload(QJsonObject json);
	void slotDownloadProgress(qint64 a, qint64 b);
	void slotDownloadFinished(QNetworkReply* reply);
	void slotPickedFile(QString fileName);
};

#endif // CARTOGRAPHERPROCESSOR_H