#include "MainWindow.h"
#include "PMZoning.h"
#include "GraphUtil.h"
#include <QFileDialog>
#include "Util.h"
#include <QElapsedTimer>

using namespace std;

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags) : QMainWindow(parent, flags) {
	ui.setupUi(this);

	connect(ui.actionLoadRoads, SIGNAL(triggered()), this, SLOT(onLoadRoads()));
	connect(ui.actionGenerateZoningByPM, SIGNAL(triggered()), this, SLOT(onGenerateZoningByPM()));
	connect(ui.actionGenerateManyZoningsByPM, SIGNAL(triggered()), this, SLOT(onGenerateManyZoningsByPM()));
	connect(ui.actionFindBestZoningByPM, SIGNAL(triggered()), this, SLOT(onFindBestZoningByPM()));

	connect(ui.actionGenerateRandomPreferences, SIGNAL(triggered()), this, SLOT(onGenerateRandomPreferences()));

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
}

MainWindow::~MainWindow() {
}

vector<pair<float, vector<float> > > MainWindow::readPreferences(const char* filename) {
	vector<pair<float, vector<float> > > preferences;

	QFile file(filename);
	file.open(QIODevice::ReadOnly);
 
	// preference vectorを読み込む
	QTextStream in(&file);
	while (true) {
		QString str = in.readLine(0);
		if (str == NULL) break;

		float weight = str.split("\t")[0].toFloat();
		QStringList preference_list = str.split("\t")[1].split(",");
		std::vector<float> preference;
		for (int i = 0; i < preference_list.size(); ++i) {
			preference.push_back(preference_list[i].toFloat());
		}

		// normalize
		Util::normalize(preference);

		preferences.push_back(make_pair(weight, preference));
	}

	return preferences;
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
	QString filename = QFileDialog::getOpenFileName(this, tr("Load preference file..."), "", tr("Preference files (*.txt)"));
	if (filename.isEmpty()) return;

	vector<pair<float, vector<float> > > preferences = readPreferences(filename.toUtf8().data());

	std::vector<float> zone_distribution(4);
	zone_distribution[0] = 0.7f; zone_distribution[1] = 0.1f; zone_distribution[2] = 0.1f; zone_distribution[3] = 0.1f;
	PMZoning pm(5000, 64, zone_distribution, roads);

	pm.initialZoning(zone_distribution);

	for (int iter = 0; iter < 40; ++iter) {
		char filename[256];
		sprintf(filename, "zoning/zone_%d.jpg", iter);
		pm.save(filename, 400);

		pm.update();
	}

	pm.computePropertyVectors();

	float score = pm.computeScore(preferences);
	printf("Score: %lf\n", score);
}

/**
 * PM方式でゾーニングを生成する
 * Celluar automata風なアルゴリズムにより、100個のゾーニングを生成する。
 */
void MainWindow::onGenerateManyZoningsByPM() {
	std::vector<float> zone_distribution(4);
	zone_distribution[0] = 0.7f; zone_distribution[1] = 0.1f; zone_distribution[2] = 0.1f; zone_distribution[3] = 0.1f;
	PMZoning pm(5000, 64, zone_distribution, roads);

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

/**
 * PM方式でたくさんのゾーニングを生成し、ベストスコアのものを探す。
 */
void MainWindow::onFindBestZoningByPM() {
	QElapsedTimer timer;
	timer.start();

	double initComputation = 0;
	double pmComputation = 0;
	double propertyComputation = 0;
	double scoreComputation = 0;

	QString filename = QFileDialog::getOpenFileName(this, tr("Load preference file..."), "", tr("Preference files (*.txt)"));
	if (filename.isEmpty()) return;

	vector<pair<float, vector<float> > > preferences = readPreferences(filename.toUtf8().data());

	std::vector<float> zone_distribution(4);
	zone_distribution[0] = 0.7f; zone_distribution[1] = 0.1f; zone_distribution[2] = 0.1f; zone_distribution[3] = 0.1f;
	PMZoning pm(5000, 64, zone_distribution, roads);

	float best_score = 0.0f;
	PMZoning best_zones(1, 1, zone_distribution, roads);
	for (int i = 0; i < 500; ++i) {
		timer.restart();
		pm.initialZoning(zone_distribution);
		initComputation += timer.elapsed();

		timer.restart();
		for (int iter = 0; iter < 40; ++iter) {
			pm.update();
		}
		pmComputation += timer.elapsed();

		timer.restart();
		pm.computePropertyVectors();
		propertyComputation += timer.elapsed();

		timer.restart();
		float score = pm.computeScore(preferences);
		scoreComputation += timer.elapsed();

		if (score > best_score) {
			best_score = score;
			best_zones = pm;
		}
	}

	best_zones.save("zoning/best_zone.jpg", 400);

	printf("init zones: %lf sec\n", initComputation / 1000.0);
	printf("PM: %lf sec\n", pmComputation / 1000.0);
	printf("property vector: %lf sec\n", propertyComputation / 1000.0);
	printf("score: %lf sec\n", scoreComputation / 1000.0);
}

/**
 * ランダムに、preferenceベクトルを10000人分作成する。
 */
void MainWindow::onGenerateRandomPreferences() {
	vector<vector<float> > preferences = Zoning::generateRandomPreferences(10000);

	QString filename = QFileDialog::getSaveFileName(this, tr("Save preference file..."), "", tr("Preference files (*.txt)"));
	if (filename.isEmpty()) return;

	QFile file(filename);
	QTextStream out(&file);
	for (int i = 0; i < preferences.size(); ++i) {
		for (int k = 0; k < preferences[i].size(); ++k) {
			if (k > 0) out << ",";
			out << preferences[i][k];
		}
		out << endl;
	}
	file.close();
}
