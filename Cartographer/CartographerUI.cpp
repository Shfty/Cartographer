#include "CartographerUI.h"
#include <QDebug>

CartographerUI::CartographerUI(QWidget *parent)
	: QObject(parent)
{
	connect(&m_webview, SIGNAL(loadFinished(bool)),
			this, SLOT(slotPageLoadFinished(bool)));

	connect(m_webview.page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(slotJavaScriptWindowObjectCleared()));

	QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	
	QString path = QDir::currentPath() + QString("/ui/debug.html");
	m_webview.load(QUrl::fromLocalFile(path));
	m_webview.show();
}

CartographerUI::~CartographerUI() {}

//Utility Functions
//Bubbles JSON up from the UI and sends it to the processor
void CartographerUI::Bubble(QByteArray dataBubble) {
		qCritical() << "Data Bubble Received";
		QJsonDocument json;
		json = json.fromJson(dataBubble);

		if(json.object().value("type").toString() == "latLon") {
			Q_EMIT(signalBeginDownload(json.object()));
		} else if(json.object().value("type").toString() == "pickFile") {
			Q_EMIT signalPickedFile(QFileDialog::getSaveFileName((QWidget*)parent(), "Save As...", "C:\\", "FBX Files (*.fbx)"));
		}
}

//Waterfalls JSON down to the UI
void CartographerUI::Waterfall(QString json) {
		m_webview.page()->mainFrame()->evaluateJavaScript("enyo.$.app.cWaterfall(" + json + ");");
}

//Slots
void CartographerUI::slotJavaScriptWindowObjectCleared() {
		qCritical() <<  "Window Object Cleared";
		m_webview.page()->mainFrame()->addToJavaScriptWindowObject("cartographer", this);
}

void CartographerUI::slotPageLoadFinished(bool ok) {
		qCritical() <<  "Load Finished. OK?" << ok;
}

void CartographerUI::slotProgressUpdate(QString message, int progress) {
		QString json;

		json += "{ progress: ";
		json += QString::number(progress);
		json += ", message: '";
		json += message;
		json += "' }";

		Waterfall(json);
}