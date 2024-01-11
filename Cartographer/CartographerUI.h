#ifndef CARTOGRAPHERUI_H
#define CARTOGRAPHERUI_H

#include <QtWidgets/QWidget>
#include <QtWebKitWidgets>

class CartographerUI : public QObject
{
	Q_OBJECT
		
public:
	CartographerUI(QWidget *parent = 0);
	~CartographerUI();
	
	Q_INVOKABLE void Bubble(QByteArray output);
	void Waterfall(QString js);

private:
	QWebView m_webview;

signals:
	void signalBeginDownload(QJsonObject json);
	void signalPickedFile(QString filename);

private slots:
	void slotJavaScriptWindowObjectCleared();
	void slotPageLoadFinished(bool ok);
	void slotProgressUpdate(QString message, int progress);
};

#endif // CARTOGRAPHERUI_H
