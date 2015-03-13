#include "MainWindow.h"
#include "PMZoning.h"
#include "GraphUtil.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags) : QMainWindow(parent, flags) {
	ui.setupUi(this);

	connect(ui.actionStart, SIGNAL(triggered()), this, SLOT(onStart()));
	connect(ui.actionLoadRoads, SIGNAL(triggered()), this, SLOT(onLoadRoads()));
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
}

MainWindow::~MainWindow() {
}

void MainWindow::onStart() {
	std::vector<float> zone_distribution(4);
	zone_distribution[0] = 0.7f; zone_distribution[1] = 0.1f; zone_distribution[2] = 0.1f; zone_distribution[3] = 0.1f;
	PMZoning pm(4000, 400, zone_distribution, roads);

	for (int i = 0; i < 100; ++i) {
		char filename[256];
		sprintf(filename, "zoning/zone_%d.jpg", i);
		pm.save(filename, 400);

		pm.update();
	}
}

void MainWindow::onLoadRoads() {
	QString filename = QFileDialog::getOpenFileName(this, tr("Open Street Map file..."), "", tr("StreetMap Files (*.gsm)"));
	if (filename.isEmpty()) return;

	GraphUtil::loadRoads(roads, filename);
}
