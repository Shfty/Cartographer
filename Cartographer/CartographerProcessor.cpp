#include <QDir>
#include <QUrl>
#include <QFile>

#include <QDebug>

#include "CartographerProcessor.h"
#include "SDK_Utility.h"

extern FbxManager *gSdkManager;     // access to the global SdkManager object
extern FbxScene      *gScene;          // access to the global scene object

QVector<QString> unparsed;		// List of un-parsed XML tags
const qreal c_coordinateScalar = 10000;		// Scale all incoming lat/lon coordinates by this value

//#define LOCAL_DATA
#undef LOCAL_DATA
//#define SMALL_MAP
#undef SMALL_MAP

//Nothing is allocated in the constructor as it gets run on the main thread
CartographerProcessor::CartographerProcessor() : QObject() {}

//This slot is invoked when the thread is started, it basically replaces the constructor
void CartographerProcessor::setup() {
//Importing
#ifdef LOCAL_DATA
#ifdef SMALL_MAP
	QString testUrl = QDir::currentPath() + QString("/testData/small.osm");
#else
	QString testUrl = QDir::currentPath() + QString("/testData/large.osm");
#endif
	QFile testFile(testUrl);

    if (!testFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
	
	parseXML(&testFile);
#else
	m_networkManager = new QNetworkAccessManager(this);
	qCritical() << "Waiting for user input";
#endif
}

CartographerProcessor::~CartographerProcessor() {
#ifndef LOCAL_DATA
	if(m_networkManager) {
		delete m_networkManager;
	}
#endif
	if(m_xmlReader) {
		delete m_xmlReader;
	}
}

void CartographerProcessor::slotBeginDownload(QJsonObject json) {
	Q_EMIT(signalProgressUpdate("Beginning Download...", 10));

	QString latLon;
	latLon += QString::number(json.value("minlon").toDouble()) + ",";
	latLon += QString::number(json.value("minlat").toDouble()) + ",";
	latLon += QString::number(json.value("maxlon").toDouble()) + ",";
	latLon += QString::number(json.value("maxlat").toDouble());
	QString urlString = "http://api.openstreetmap.org/api/0.6/map?bbox=" + latLon;
	QUrl url = QUrl::fromEncoded(urlString.toLocal8Bit());
    QNetworkRequest request(url);

	connect(m_networkManager->get(request), SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(slotDownloadProgress(qint64,qint64)));
	connect(m_networkManager, SIGNAL(finished(QNetworkReply*)),this, SLOT(slotDownloadFinished(QNetworkReply*)));
}

void CartographerProcessor::slotDownloadProgress(qint64 received, qint64 total) {
	if(total != -1) {
		QString message = "Download Progress: " + QString::number(received) + "b / " + QString::number(total) + "b";
		qCritical() << message;
		Q_EMIT(signalProgressUpdate(message, 20));
	}
}

void CartographerProcessor::slotDownloadFinished(QNetworkReply* reply) {
	Q_EMIT(signalProgressUpdate("Download Complete", 30));
	qCritical() << "Download Complete";
	parseXML(reply);
}

void CartographerProcessor::parseXML(QIODevice* xml) {
	Q_EMIT(signalProgressUpdate("Parsing XML...", 40));
	qCritical() << "Parsing XML...";
	m_xmlReader = new QXmlStreamReader(xml);

	if (m_xmlReader->readNextStartElement()) {
		if (m_xmlReader->name() == "osm" && m_xmlReader->attributes().value("version") == "0.6") {
			qCritical() << "Verified OSM v0.6";
			parseOSM();
		}
		else {
			m_xmlReader->raiseError(QObject::tr("The file is not an OSM version 0.6 file."));
		}
	}
}

void CartographerProcessor::parseOSM() {
	Q_EMIT(signalProgressUpdate("Parsing OSM...", 50));
	qCritical() << "Parsing OSM...";
	Q_ASSERT(m_xmlReader->isStartElement() && m_xmlReader->name() == "osm");

    while (m_xmlReader->readNextStartElement()) {
        if (m_xmlReader->name() == "bounds") {
            parseBounds();
		}
        else if (m_xmlReader->name() == "node") {
            parseNode();
		}
        else if (m_xmlReader->name() == "way") {
            parseWay();
		}
        else {
			if(!unparsed.contains(m_xmlReader->name().toString())) {
				unparsed.append(m_xmlReader->name().toString());
			}

			m_xmlReader->skipCurrentElement();
		}
    }
	
	Q_EMIT(signalProgressUpdate("Parsing Complete", 60));
	qCritical() << "Parsing Complete";

	qCritical() << "\nCamera Target:" << m_cameraTarget.mData[0] << m_cameraTarget.mData[2];

	qCritical() << "\nNodes:";
	Q_FOREACH(quint64 value, m_nodes.keys()) {
		qCritical() << "ID:" << value << "Position:" << m_nodes[value];
	}
	qCritical();
	
	Q_EMIT(signalProgressUpdate("Setting up WorldObjects...", 70));
	qCritical() << "Setting up WorldObjects...";
	Q_FOREACH(WorldObject* object, m_worldObjects.toList()) {
		//Load vertices into WorldObjects
		Q_FOREACH(quint64 index, object->VertexRefs()->toList()) {
			object->VerticesFootprint()->append(m_nodes[index]);
		}

		//Calculate World Transform
		object->CalcWorldTrans();
		
		//Reverse transform the vertices against the world to get local space
		object->Localize();

		//Extrude into 3D
		object->Extrude();
	}

	qCritical() << "\nObjects:";
	Q_FOREACH(WorldObject* object, m_worldObjects) {
		qCritical() << "ID:" << object->GetId();
		qCritical() << "Username:" << object->GetUsername();
		qCritical() << "World Transform:" << object->WorldTrans()->mData[0] << object->WorldTrans()->mData[1] << object->WorldTrans()->mData[2];
		qCritical() << "Vertex References:" << object->VertexRefs()->toList();
		qCritical() << "Footprint Vertices:" << object->VerticesFootprintLocal()->toList();
		qCritical() << "3D Vertices:";
		Q_FOREACH(FbxVector4 vertex, object->Vertices3D()->toList()) {
			qCritical() << vertex.mData[0] << vertex.mData[1] << vertex.mData[2];
		}
		qCritical() << "Metadata:";
		for(int i = 0; i < object->Metadata()->count(); i++) {
			qCritical() << object->Metadata()->keys()[i] << object->Metadata()->values()[i];
		}
		qCritical();
	}

	qCritical() << "\nUnparsed elements:";
	Q_FOREACH(QString string, unparsed) {
		qCritical() << string;
	}
	
	qCritical() << "";

	composeScene();
}

void CartographerProcessor::parseBounds() {
	Q_ASSERT(m_xmlReader->isStartElement() && m_xmlReader->name() == "bounds");

	qreal minlat, minlon, maxlat, maxlon;

	Q_FOREACH(QXmlStreamAttribute attribute, m_xmlReader->attributes()) {
		if(attribute.name() == "minlat") {
			minlat = attribute.value().toString().toFloat();
		}
		else if(attribute.name() == "minlon") {
			minlon = attribute.value().toString().toFloat();
		}
		else if(attribute.name() == "maxlat") {
			maxlat = attribute.value().toString().toFloat();
		}
		else if(attribute.name() == "maxlon") {
			maxlon = attribute.value().toString().toFloat();
		}
	}

	m_cameraTarget = FbxVector4((minlat + maxlat) / 2, 0, (minlon + maxlon) / 2);

    m_xmlReader->skipCurrentElement();
}

void CartographerProcessor::parseNode() {
	Q_ASSERT(m_xmlReader->isStartElement() && m_xmlReader->name() == "node");

	quint64 id;
	QVector2D position;
	bool visible;

	Q_FOREACH(QXmlStreamAttribute attribute, m_xmlReader->attributes()) {
		if(attribute.name() == "id") {
			id = attribute.value().toString().toULong();
		}
		else if(attribute.name() == "lat") {
			position.setX(attribute.value().toString().toFloat());
		}
		else if(attribute.name() == "lon") {
			position.setY(attribute.value().toString().toFloat());
		}
		else if(attribute.name() == "visible") {
			visible = attribute.value() == "true" ? true : false;
		}
	}

	if(visible == true) {
		m_nodes.insert(id, position);

		while (m_xmlReader->readNextStartElement()) {
			if (m_xmlReader->name() == "tag") {
				parseTag();
			}
			else {
				if(!unparsed.contains(m_xmlReader->name().toString())) {
					unparsed.append(m_xmlReader->name().toString());
				}

				m_xmlReader->skipCurrentElement();
			}
		}
	}
}

void CartographerProcessor::parseWay() {
	Q_ASSERT(m_xmlReader->isStartElement() && m_xmlReader->name() == "way");

	quint64 id;
	QString uname;
	bool visible;

	Q_FOREACH(QXmlStreamAttribute attribute, m_xmlReader->attributes()) {
		if(attribute.name() == "id") {
			id = attribute.value().toString().toInt();
		}
		if(attribute.name() == "user") {
			uname = attribute.value().toString();
		}
		else if(attribute.name() == "visible") {
			visible = attribute.value() == "true" ? true : false;
		}
	}

	if(visible == true) {
		WorldObject* object;
		object = new WorldObject();
		object->SetId(id);
		object->SetUsername(uname);
	
		while (m_xmlReader->readNextStartElement()) {
			if (m_xmlReader->name() == "nd") {
				parseNd(object);
			}
			else if (m_xmlReader->name() == "tag") {
				parseTag(object);
			}
			else {
				if(!unparsed.contains(m_xmlReader->name().toString())) {
					unparsed.append(m_xmlReader->name().toString());
				}

				m_xmlReader->skipCurrentElement();
			}
		}

		m_worldObjects.append(object);
	}
}

void CartographerProcessor::parseNd(WorldObject* object) {
	Q_ASSERT(m_xmlReader->isStartElement() && m_xmlReader->name() == "nd");

	qCritical() << "Parsing Nd...";
	Q_FOREACH(QXmlStreamAttribute attribute, m_xmlReader->attributes()) {
		quint64 ref;

		if(attribute.name() == "ref") {
			ref = attribute.value().toString().toULong();
		}

		object->VertexRefs()->append(ref);
	}

    m_xmlReader->skipCurrentElement();
}

void CartographerProcessor::parseTag() {
	Q_ASSERT(m_xmlReader->isStartElement() && m_xmlReader->name() == "tag");

	qCritical() << "Parsing Tag...";
	Q_FOREACH(QXmlStreamAttribute attribute, m_xmlReader->attributes()) {
		qCritical() << "\t" << attribute.name() << ":" << attribute.value();
	}
    m_xmlReader->skipCurrentElement();
}

void CartographerProcessor::parseTag(WorldObject* object) {
	Q_ASSERT(m_xmlReader->isStartElement() && m_xmlReader->name() == "tag");

	qCritical() << "Parsing Tag...";

	QString key;
	QString value;

	Q_FOREACH(QXmlStreamAttribute attribute, m_xmlReader->attributes()) {
		if(attribute.name().toString() == "k") {
			key = attribute.value().toString();
		}
		else if(attribute.name().toString() == "v") {
			value = attribute.value().toString();
		}
	}
	
	Q_ASSERT(key != NULL);
	Q_ASSERT(value != NULL);

	qCritical() << "Adding metadata:" << key << ":" << value;
	object->Metadata()->insert(key, value);

    m_xmlReader->skipCurrentElement();
}

void CartographerProcessor::composeScene() {
	Q_EMIT(signalProgressUpdate("Composing FBX Scene...", 85));
	qCritical() << "Composing FBX Scene...";

	if(gSdkManager == NULL)
    {
        // Create Scene
        CreateScene();
		
		// Add Objects
		int idx = 0;
		Q_FOREACH(WorldObject* object, m_worldObjects) {
			createWorldObject(idx, object);
			idx++;
		}
	
		Q_EMIT(signalProgressUpdate("Processing Complete", 100));
		qCritical() << "Processing Complete";
    }
}

void CartographerProcessor::slotPickedFile(QString fileName) {
	if(fileName != NULL) {
		Export((fileName).toLocal8Bit().data(), 0);
	} else {
		qCritical() << "Save As Canceled";
	}
	emit finished();
}

// to create a basic scene
bool CartographerProcessor::createScene()
{
    // Initialize the FbxManager and the FbxScene
    if(InitializeSdkObjects(gSdkManager, gScene) == false)
    {
        return false;
    }

    // create a single texture shared by all cubes
    CreateTexture(gScene);

    // create a material shared by all faces of all cubes
    CreateMaterial(gScene);

    return true;
}

void CartographerProcessor::createWorldObject(int idx, WorldObject* object)
{
	//qCritical() << "Creating World Object";

	// Sequentially name the objects
	FbxString lCubeName = "WorldObject #";
    lCubeName += FbxString(idx);

	FbxNode* lObject = createObjectMesh(gScene, lCubeName.Buffer(), object);
	
	// Calculate scaled positions
	FbxVector4 scaledWorldTrans(object->WorldTrans()->mData[0] * c_coordinateScalar, object->WorldTrans()->mData[1] * c_coordinateScalar, object->WorldTrans()->mData[2] * c_coordinateScalar);
	FbxVector4 scaledCameraTarget(m_cameraTarget.mData[0] * c_coordinateScalar, m_cameraTarget.mData[1] * c_coordinateScalar, m_cameraTarget.mData[2] * c_coordinateScalar);

	int objectHeight = 1.0f;

	qCritical() << object->Metadata()->keys();

	if(object->Metadata()->keys().contains("landuse")) {
		qCritical() << "Parsed Land Use";
		objectHeight = 0.5f;
	}
	else if(object->Metadata()->keys().contains("highway")) {
		qCritical() << "Parsed Highway";
		objectHeight = 2.0f;
	}
	else if(object->Metadata()->keys().contains("railway")) {
		qCritical() << "Parsed Railway";
		objectHeight = 2.0f;
	}
	else if(object->Metadata()->keys().contains("building")) {
		qCritical() << "Parsed Building";
		objectHeight = 10.0f;
	}

	// Apply position and scaling
	lObject->LclTranslation.Set(scaledWorldTrans - scaledCameraTarget); //Offset by camera target for a more accurate origin point
    lObject->LclScaling.Set(FbxVector4(c_coordinateScalar, objectHeight, c_coordinateScalar));

	gScene->GetRootNode()->AddChild(lObject);
}

//#define FOOTPRINT_GEOMETRY
#undef FOOTPRINT_GEOMETRY

FbxNode* CartographerProcessor::createObjectMesh(FbxScene* pScene, char* pName, WorldObject* object)
{
	qCritical() << "Creating Object Mesh";
    FbxMesh* lMesh = FbxMesh::Create(pScene,pName);

#ifdef FOOTPRINT_GEOMETRY //Use the objects' flat footprint (roads etc)
    // Create control points.
    lMesh->InitControlPoints(object->verticesFootprintLocal()->count());
    FbxVector4* lControlPoints = lMesh->GetControlPoints();

	for(int i = 0; i < object->verticesFootprintLocal()->count(); i++) {
		lControlPoints[i] = FbxVector4(object->verticesFootprintLocal()->at(i).x(), 0, object->verticesFootprintLocal()->at(i).y());
		qCritical() << object->verticesFootprintLocal()->at(i).x() << 0 << object->verticesFootprintLocal()->at(i).y();
	}
	
    // all faces of the cube have the same texture
    lMesh->BeginPolygon(-1, -1, -1, false);

    for(int i = 0; i < object->verticesFootprintLocal()->count(); i++)
    {
        lMesh->AddPolygon(i);
    }

    lMesh->EndPolygon();
#else //Use extruded version
    // Create control points.
    lMesh->InitControlPoints(object->Vertices3D()->count());
    FbxVector4* lControlPoints = lMesh->GetControlPoints();

	for(int i = 0; i < object->Vertices3D()->count(); i++) {
		lControlPoints[i] = object->Vertices3D()->at(i);
	}

    for(int i = 0; i < object->Vertices3D()->count() - 3; i+=2)
    {
		lMesh->BeginPolygon(-1, -1, -1, false);
		
        lMesh->AddPolygon(i + 2);
        lMesh->AddPolygon(i + 1);
        lMesh->AddPolygon(i);

		lMesh->EndPolygon();

		lMesh->BeginPolygon(-1, -1, -1, false);
		
        lMesh->AddPolygon(i + 3);
        lMesh->AddPolygon(i + 1);
        lMesh->AddPolygon(i + 2);

		lMesh->EndPolygon();
    }
	
	lMesh->BeginPolygon(-1, -1, -1, false);
    for(int i = 1; i < object->Vertices3D()->count(); i+=2)
    {
        lMesh->AddPolygon(i);
    }
	lMesh->EndPolygon();
#endif

    // create a FbxNode
    FbxNode* lNode = FbxNode::Create(pScene,pName);

    // set the node attribute
    lNode->SetNodeAttribute(lMesh);

    // set the shading mode to view texture
    lNode->SetShadingMode(FbxNode::eTextureShading);

    // return the FbxNode
    return lNode;
}