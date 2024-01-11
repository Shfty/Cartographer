#include "Cartographer.h"
#include <QDebug>

Cartographer::Cartographer(QWidget *parent)
	: QWidget(parent)
{
	//Create the processor and push it onto a separate thread
	QThread* processorThread = new QThread;
	m_processor = new CartographerProcessor();
	m_processor->moveToThread(processorThread);
	
	//Hook up thread start/finish signals
	connect(processorThread, SIGNAL(started()), m_processor, SLOT(setup()));
	connect(m_processor, SIGNAL(finished()), processorThread, SLOT(quit()));
	connect(m_processor, SIGNAL(finished()), m_processor, SLOT(deleteLater()));
	connect(processorThread, SIGNAL(finished()), processorThread, SLOT(deleteLater()));
	connect(m_processor, SIGNAL(finished()), this, SLOT(exitLater()));

	//Hook up UI <-> Processor communication
	connect(&m_ui, SIGNAL(signalBeginDownload(QJsonObject)), m_processor, SLOT(slotBeginDownload(QJsonObject)));
	connect(m_processor, SIGNAL(signalProgressUpdate(QString, int)), &m_ui, SLOT(slotProgressUpdate(QString, int)));
	connect(&m_ui, SIGNAL(signalPickedFile(QString)), m_processor, SLOT(slotPickedFile(QString)));

	//Start
	processorThread->start();
}

//Not deleting m_processor in the constructor, as it's been marked for later deletion
Cartographer::~Cartographer() {}

void Cartographer::exitLater() {
	//Process any remaining events to make sure everything gets cleaned up
	while(QApplication::hasPendingEvents()) {
		QApplication::processEvents();
	}

	//Exit
	QApplication::exit();
}