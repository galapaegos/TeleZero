#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <filesystem>

#include "camera.h"
#include "tiff.h"
#include "util.h"

#include <QFileDialog>
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
    : QMainWindow(parent), begin_capture(0), captured_images(0), total_images(0), current_sequence(-1)
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
	
	temperature_info = new QLabel("Sensor Temperature: ", this);
	temperature_info->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	
	sequence_info = new QLabel("Sequence: 0", this);
	sequence_info->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	
	statusBar()->addPermanentWidget(temperature_info, 1);
	statusBar()->addPermanentWidget(sequence_info, 1);
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
	
	QObject::disconnect(&view_idle_timer, &QTimer::timeout, this, &MainWindow::update_view);
	
	camera->disconnect_camera();
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
	
	if(pixel_format == "XRGB8888") {
		ui->view->color_order = 3;
	}
	if(pixel_format == "XBGR8888") {
		ui->view->color_order = 2;
	}
	if(pixel_format == "RGBA8888") {
		ui->view->color_order = 0;
	}
	if(pixel_format == "BGRA8888") {
		ui->view->color_order = 1;
	}
	if(pixel_format == "BGR888") {
		ui->view->color_order = 1;
	}
	if(pixel_format == "RGB888") {
		ui->view->color_order = 0;
	}
}

void MainWindow::on_camera_gain_valueChanged()
{
	camera->analogue_gain = ui->camera_gain->value();
	printf("Analogue Gain: %0.2f\n", camera->analogue_gain);
}

void MainWindow::on_camera_exposure_valueChanged()
{
	// Converting from milliseconds to microseconds;
	camera->exposure_time = ui->camera_exposure->value() * 1000;
	printf("Exposure Time: %i ms\n", camera->exposure_time);
}

void MainWindow::on_camera_brightness_valueChanged()
{
	camera->brightness = ui->camera_brightness->value();
	printf("Brightness: %0.2f\n", camera->brightness);
}

void MainWindow::on_camera_contrast_valueChanged()
{
	camera->contrast = ui->camera_contrast->value();
	printf("Contrast: %0.2f\n", camera->contrast);
}

void MainWindow::on_camera_saturation_valueChanged()
{
	camera->saturation = ui->camera_saturation->value();
	printf("Saturation: %0.2f\n", camera->saturation);
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
	
	if(camera->channels == 3) {
		ui->view->set_texture_type(GL_RGB8UI);
	}
	
	if(camera->channels == 4) {
		ui->view->set_texture_type(GL_RGBA8UI);
	}
	
	camera->configure_camera(pixel_format_index, pixel_format_size_index);
}

void MainWindow::on_start_camera_clicked()
{
	printf("Starting camera...\n");
	camera->start_camera();
	
	QObject::connect(&view_idle_timer, &QTimer::timeout, this, &MainWindow::update_view);
	
	view_idle_timer.start();
	view_idle_timer.setInterval(50);
}

void MainWindow::on_stop_camera_clicked()
{
	printf("Stopping camera...\n");
	
	view_idle_timer.stop();
	
	QObject::disconnect(&view_idle_timer, &QTimer::timeout, this, &MainWindow::update_view);
	
	camera->stop_camera();
}

void MainWindow::on_crosshair_clicked(){
	ui->view->set_crosshair_visible(ui->crosshair->isChecked());
}

void MainWindow::on_capture_path_clicked()
{
	auto directory = QFileDialog::getExistingDirectory().toStdString();
	
	if(directory.compare("") == 0) {
		return;
	}
	
	ui->storage_path->setText(QString::fromStdString(directory));
}

void MainWindow::on_capture_begin_clicked()
{
	auto storage_path = ui->storage_path->text().toStdString();
	auto session_name = ui->session_name->text().toStdString();
	auto session_id = ui->session_id->text().toStdString();
	total_images = ui->image_captures->value();
	
	if(session_name.compare("") == 0 || session_id.compare("") == 0) {
		printf("Please specify session name and id.\n");
		return;
	}

	// Get time stamp
	const std::time_t result = std::time(nullptr);
	auto tm = std::localtime(&result);

	auto time_stamp = format("%04i_%02i_%02i", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	printf("time_stamp:%s\n", time_stamp.c_str());
	
	session_path = format("%s/%s/%s/%s", storage_path.c_str(), time_stamp.c_str(), session_name.c_str(), session_id.c_str());
	
	printf("Session_path: %s\n", session_path.c_str());
	
	std::filesystem::create_directories(session_path);
	
	begin_capture = 1;
	captured_images = 0;
}

void MainWindow::on_capture_cancel_clicked(){
	begin_capture = 0;
}

void MainWindow::update_view() {
	if(camera->sequence == current_sequence) {
		return;
	}

	current_sequence = camera->sequence;
	
	std::vector<uint8_t> buffer;
	camera->get_image(buffer);
	
	if(begin_capture) {
		if(captured_images < total_images) {
			auto duplicate_buffer = buffer;
			if(camera->channels == 3) {}
			if(camera->channels == 4) {
				switch(ui->view->color_order) {
					case 3:
					{
						for(int64_t y = 0; y < camera->height; y++) {
							for(int64_t x = 0; x < camera->width; x++) {
								int64_t idx = (y * camera->width + x)*4;
								std::swap(duplicate_buffer[idx + 0], duplicate_buffer[idx + 2]);
							}
						}
					}break;
				}
			}
			auto file = format("%s/image_%0.4i.tif", session_path.c_str(), captured_images);
			write_tiff(file, camera->width, camera->height, camera->channels, duplicate_buffer);
		
			captured_images++;
			printf("captured %i of %i\n", captured_images, total_images);
		} else {
			printf("capture complete\n");
			begin_capture = 0;
		}
	}
	
	// Update info
	temperature_info->setText(QString::fromStdString(format("Sensor Temperature: %0.2fC  %0.2fF", camera->temperature, (camera->temperature * 9 / 5) + 32.f)));
	sequence_info->setText(QString::fromStdString(format("Sequence: %lld", camera->sequence)));
	
	ui->view->set_buffer(camera->width, camera->height, camera->channels, buffer);
	ui->view->repaint();
}
