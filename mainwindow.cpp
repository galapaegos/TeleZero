#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "camera.h"

#include <QMessageBox>

std::vector<int> get_selected_rows(QList<QTableWidgetItem *> list)
{
    std::set<int> selected;

    for(int i = 0; i < list.size(); i++) {
        selected.insert(list[i]->row());
    }

    std::vector<int> results(selected.size());
    std::copy(selected.begin(), selected.end(), results.begin());

    return results;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
	ui = std::make_unique<Ui::MainWindow>();
    ui->setupUi(this);

    camera = std::make_unique<Camera>();
    camera->initialize();

    auto cameras = camera->get_cameras();
    for(auto s = 0; s < cameras.size(); s++) {
        printf("name: %s\n", cameras[s].c_str());
    }
	
	QStringList cameras_header;
	cameras_header.push_back("Camera Name");
	
	ui->camera_list->setColumnCount(cameras_header.size());
	ui->camera_list->setHorizontalHeaderLabels(cameras_header);

	ui->camera_list->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
	ui->camera_list->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);

	ui->camera_list->setEditTriggers(QAbstractItemView::NoEditTriggers);

	for(auto i = 0; i < cameras.size(); i++) {
		int idx = ui->camera_list->rowCount();
		ui->camera_list->insertRow(idx);

		ui->camera_list->setItem(idx, 0, new QTableWidgetItem(QString::fromStdString(cameras[i])));
	}
	
	ui->pixel_format->addItem(QString::fromStdString(convert_format(libcamera::formats::YUV420)));
}

MainWindow::~MainWindow()
{
    camera->uninitialize();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	// Prompt are you sure you want to continue?
	auto reply = QMessageBox::question(this, "Quit?", "Would you like to close?", QMessageBox::Yes | QMessageBox::No);

	if(reply == QMessageBox::No) {
		event->ignore();
		return;
	}
}

void MainWindow::on_connect_camera_clicked()
{
	auto selected_items = ui->camera_list->selectedItems();
	auto selected_rows = get_selected_rows(selected_items);
	
	if(selected_rows.size() != 1) {
		printf("No camera selected\n");
		return;
	}
	
	auto selected_camera = ui->camera_list->item(selected_rows[0], 0)->text().toStdString();
	camera->connect_camera(selected_camera);
}

void MainWindow::on_disconnect_camera_clicked()
{
	camera->disconnect_camera();
}

void MainWindow::on_camera_gain_valueChanged(){}
void MainWindow::on_camera_exposure_valueChanged(){}

void MainWindow::on_configure_camera_clicked()
{
	auto width = ui->camera_width->value();
	auto height = ui->camera_height->value();
	auto format = convert_format(ui->pixel_format->currentText().toStdString());
	
	camera->configure_camera(width, height, format);
}

void MainWindow::on_start_camera_clicked()
{
	camera->start_camera();
	
	QObject::connect(&view_idle_timer, &QTimer::timeout, this, &MainWindow::update_view);
	
	view_idle_timer.start();
	view_idle_timer.setInterval(50);
}

void MainWindow::on_stop_camera_clicked()
{
	QObject::disconnect(view_idle_timer);
	
	camera->stop_camera();
}

void MainWindow::capture_path_clicked(){}
void MainWindow::capture_begin_clicked(){}
void MainWindow::capture_cancel_clicked(){}

void MainWindow::update_view() {
	auto width = ui->camera_width->value();
	auto height = ui->camera_height->value();
	
	std::vector<uint8_t> buffer;
	camera->grab_frame(buffer);
	
	ui->view->set_buffer(width, height, buffer);
	ui->view->repaint();
}
