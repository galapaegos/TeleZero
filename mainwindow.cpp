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
	
	temperature_info = new QLabel("", this);
	temperature_info->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	
	statusBar()->addPermanentWidget(temperature_info, 1);
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
	
	auto formats = camera->get_pixel_formats();
	ui->pixel_format->clear();
	for(auto i = 0; i < formats.size(); i++) {
		ui->pixel_format->addItem(QString::fromStdString(formats[i]));
	}
	ui->pixel_format->setStyleSheet("combobox-popup: 0;");
}

void MainWindow::on_disconnect_camera_clicked()
{
	camera->disconnect_camera();
}

void MainWindow::on_pixel_format_currentIndexChanged(int index)
{
	if(index < 0) {
		return;
	}
	
	auto pixel_formats = camera->get_pixel_formats();
	auto pixel_format = pixel_formats[index];
	
	auto sizes = camera->get_pixel_format_sizes(pixel_format);
	
	ui->format_size->clear();
	for(auto i = 0; i < sizes.size(); i++) {
		ui->format_size->addItem(QString::fromStdString(sizes[i]));
	}
	ui->format_size->setStyleSheet("combobox-popup: 0;");
}

void MainWindow::on_camera_gain_valueChanged()
{
	camera->analogue_gain = ui->camera_gain->value();
}

void MainWindow::on_camera_exposure_valueChanged()
{
	camera->exposure_time = ui->camera_exposure->value();
}

void MainWindow::on_camera_brightness_valueChanged()
{
	camera->brightness = ui->camera_brightness->value();
}

void MainWindow::on_camera_contrast_valueChanged()
{
	camera->contrast = ui->camera_contrast->value();
}

void MainWindow::on_camera_saturation_valueChanged()
{
	camera->saturation = ui->camera_saturation->value();
}

void MainWindow::on_camera_sharpness_valueChanged()
{
	camera->sharpness = ui->camera_sharpness->value();
}

void MainWindow::on_lens_position_valueChanged()
{
	camera->lens_position = ui->lens_position->value();
}

void MainWindow::on_configure_camera_clicked()
{
	auto pixel_format_index = ui->pixel_format->currentIndex();
	auto pixel_format_size_index = ui->format_size->currentIndex();
	
	if(pixel_format_index < 0) {
		return;
	}
	
	if(pixel_format_size_index < 0) {
		return;
	}
	
	camera->configure_camera(pixel_format_index, pixel_format_size_index);
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
	QObject::disconnect(&view_idle_timer, &QTimer::timeout, this, &MainWindow::update_view);
	
	camera->stop_camera();
}

void MainWindow::capture_path_clicked(){}
void MainWindow::capture_begin_clicked(){}
void MainWindow::capture_cancel_clicked(){}

void MainWindow::update_view() {
	//auto width = ui->camera_width->value();
	//auto height = ui->camera_height->value();
	
	//printf("MainWindow::update_view()\n");
	
	int width, height;
	std::vector<uint8_t> buffer;
	camera->get_image(width, height, buffer);
	
	temperature_info->setText(QString::number(camera->temperature));
	
	ui->view->set_buffer(width, height, 4, buffer);
	ui->view->repaint();
}
