#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include "ui_MainWindow.h"
#include "RoadGraph.h"

using namespace std;

class MainWindow : public QMainWindow {
	Q_OBJECT

private:
	Ui::MainWindowClass ui;
	RoadGraph roads;

public:
	MainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
	~MainWindow();

	vector<pair<float, vector<float> > > readPreferences(const char* filename);

public slots:
	void onLoadRoads();
	void onGenerateZoningByPM();
	void onGenerateManyZoningsByPM();
	void onFindBestZoningByPM();
	void onGenerateRandomPreferences();
};

#endif // MAINWINDOW_H
