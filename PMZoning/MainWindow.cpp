#include "MainWindow.h"
#include "PMZoning.h"
#include "GraphUtil.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags) : QMainWindow(parent, flags) {
	ui.setupUi(this);

	connect(ui.actionLoadRoads, SIGNAL(triggered()), this, SLOT(onLoadRoads()));
	connect(ui.actionGenerateZoningByPM, SIGNAL(triggered()), this, SLOT(onGenerateZoningByPM()));
	connect(ui.actionGenerateManyZoningsByPM, SIGNAL(triggered()), this, SLOT(onGenerateManyZoningsByPM()));
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
}

MainWindow::~MainWindow() {
}

void MainWindow::onLoadRoads() {
	QString filename = QFileDialog::getOpenFileName(this, tr("Open Street Map file..."), "", tr("StreetMap Files (*.gsm)"));
	if (filename.isEmpty()) return;

	GraphUtil::loadRoads(roads, filename);
}

/**
 * PM方式でゾーニングを生成する
 * Celluar automata風なアルゴリズムにより、ゾーニングを生成する。
 */
void MainWindow::onGenerateZoningByPM() {
	std::vector<float> zone_distribution(4);
	zone_distribution[0] = 0.7f; zone_distribution[1] = 0.1f; zone_distribution[2] = 0.1f; zone_distribution[3] = 0.1f;
	PMZoning pm(5000, 100, zone_distribution, roads);

	pm.initialZoning(zone_distribution);

	for (int i = 0; i < 100; ++i) {
		char filename[256];
		sprintf(filename, "zoning/zone_%d.jpg", i);
		pm.save(filename, 400);

		pm.update();
	}
}

/**
 * PM方式でゾーニングを生成する
 * Celluar automata風なアルゴリズムにより、100個のゾーニングを生成する。
 */
void MainWindow::onGenerateManyZoningsByPM() {
	std::vector<float> zone_distribution(4);
	zone_distribution[0] = 0.7f; zone_distribution[1] = 0.1f; zone_distribution[2] = 0.1f; zone_distribution[3] = 0.1f;
	PMZoning pm(5000, 100, zone_distribution, roads);

	for (int i = 0; i < 100; ++i) {
		pm.initialZoning(zone_distribution);

		for (int iter = 0; iter < 40; ++iter) {
			pm.update();
		}
		char filename[256];
		sprintf(filename, "zoning/zone_%d.jpg", i);
		pm.save(filename, 400);
	}
}

