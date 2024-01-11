#ifndef CARTOGRAPHER_H
#define CARTOGRAPHER_H

#include <QtWidgets/QWidget>

#include "CartographerProcessor.h"
#include "CartographerUI.h"

class Cartographer : public QWidget
{
	Q_OBJECT
		
public:
	Cartographer(QWidget *parent = 0);
	~Cartographer();

private:
	CartographerUI m_ui;
	CartographerProcessor* m_processor;

private slots:
	void exitLater();
};

#endif // CARTOGRAPHER_H
