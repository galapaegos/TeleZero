#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>

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
	
	void closeEvent(QCloseEvent *event);
	
	void update_view();
	
private Q_SLOTS:
	void on_connect_camera_clicked();
	void on_disconnect_camera_clicked();
	
	void on_pixel_format_currentIndexChanged(int index);
	
	void on_camera_gain_valueChanged();
	void on_camera_exposure_valueChanged();
	void on_camera_brightness_valueChanged();
	void on_camera_contrast_valueChanged();
	void on_camera_saturation_valueChanged();
	
	void on_configure_camera_clicked();
	
	void on_start_camera_clicked();
	void on_stop_camera_clicked();

	void on_crosshair_clicked();
	
	void on_capture_path_clicked();
	void on_capture_begin_clicked();
	void on_capture_cancel_clicked();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<Camera> camera;
	
	int begin_capture;
	int toss_frames;
	int captured_images;
	int total_images;
	int current_sequence;
	std::string session_path;
	QLabel *temperature_info;
	QLabel *sequence_info;
	
	QTimer view_idle_timer;
};
#endif // MAINWINDOW_H
