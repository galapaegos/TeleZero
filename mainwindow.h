#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>

#include "camera.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

std::vector<int> get_selected_rows(QList<QTableWidgetItem *> list);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
	
private Q_SLOTS:
	void on_connect_camera_clicked();
	void on_disconnect_camera_clicked();
	
	void on_camera_gain_valueChanged();
	void on_camera_exposure_valueChanged();
	
	void on_configure_camera_clicked();
	
	void capture_path_clicked();
	void capture_begin_clicked();
	void capture_cancel_clicked();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<Camera> camera;
};
#endif // MAINWINDOW_H
