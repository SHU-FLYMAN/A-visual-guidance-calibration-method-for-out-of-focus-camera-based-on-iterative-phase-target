/* UI�������棨���ڴ��������źţ�*/ 

#pragma once
#include <fstream>
#include <QtWidgets/QMainWindow>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QKeyEvent>
#include <QMutex>
#include <QMessageBox>

#include "ui_UI.h"
#include "CameraEngineer.h"
#include "Calibrator.h"


class UI : public QMainWindow
{
    Q_OBJECT

public:
	Ui::UIClass ui;
	Camera_Flycapture cap;  // ������ͣ������滻
	vector<string> filenames;   // ����ͶӰͼƬ
	Calibrator calibrator;
	string save_folder = "./out";
	string img_end     = ".bmp";
	int folder_count = 0;
	QMutex mutex;
	
public:
	UI(QWidget *parent = Q_NULLPTR) : QMainWindow(parent) {
		ui.setupUi(this);
		// ���ù̶���С
		setFixedSize(960, 900);

		// ��Ϊ����֧��Mat���ݣ����Ҫע��һ��
		qRegisterMetaType<Mat>("Mat");
		connect(CameraEngineer::Get(this->cap),
			SIGNAL(img_signal(Mat)),  // ��������
			ui.imgWindow,
			SLOT(imshow_slot(Mat))    // ��������
		);
	};

	// ��дcloseEvent: ȷ���˳��Ի���
	void closeEvent(QCloseEvent *event)
	{	
		CameraEngineer::Get(this->cap)->disconnect();        // �Ͽ��������
		CameraEngineer::Get(this->cap)->stop_flag = true;    // �˳�ѭ��
	}

	// ͶӰ����
	void function_project(const string &filename) {
		const char *file = filename.data();
		SystemParametersInfoA(SPI_SETDESKWALLPAPER, 1, (PVOID)file, SPIF_SENDCHANGE);
	}

// �ۺ���
public slots:
	/* ������� */
	void slot_camera_connect() {
		// �ȶϿ�֮ǰ���
		CameraEngineer::Get(this->cap)->disconnect();

		// ���������
		this->cap.connect((unsigned int)ui.spinBox_ID->value());

		this->cap.setShutter(ui.spinBox_shutter->value());
		this->cap.setGain(ui.spinBox_gain->value());

		// ���������ع⣨����ʱ�򲻱䣩

		connect(ui.spinBox_shutter,
			QOverload<int>::of(&QSpinBox::valueChanged),
			[=](int shutter) {
			this->cap.setShutter(shutter);
		});

		// ������������
		connect(ui.spinBox_gain,
			QOverload<double>::of(&QDoubleSpinBox::valueChanged),
			[=](double X) {
			this->cap.setGain(X);
		});
	}

	/* ������� */
	void slot_camrea_capture() {
		// Ĭ�ϱ�����out�ļ���
		CameraEngineer::Get(this->cap)->captureSaveMutex(save_folder, img_end);
		}

	/* д��ͼƬ */
	void slot_screen_write() {
		QStringList files = QFileDialog::getOpenFileNames(
			this,
			QString::fromLocal8Bit("Open images"),
			QDir::currentPath() + "/patterns",
			tr("images files(*.bmp *.jpg *.png)"));
		if (files.isEmpty())
		{
			return;
		}
		// ����ļ�
		filenames.resize(0);
		cout << "Successfully load images:" << endl;
		for (int i = 0; i < files.size(); i++)
		{
			QString file = files.at(i);
			qDebug() << file << endl;
			string filename = file.toStdString();
			filenames.emplace_back(filename);
		}
		// ͶӰ��һ��ͼƬ��Ϊ����
		function_project(filenames[0]);
	}

	/* ͶӰ������ */
	void slot_project_capture() {
		if (filenames.size() < 1)
		{
			cout << "No images written!" << endl;
			return;
		}
		folder_count += 1;
		// λ��ȷ����ֹͣ���
		CameraEngineer::Get(this->cap)->stop_detect();
		cout << "Start to project images:" << endl;

		// ��ͼƬ�������Ϊ0
		CameraEngineer::Get(this->cap)->setImageCountZeros();
		for (int i = 0; i < filenames.size(); i++)
		{
			// ͶӰͼƬ
			function_project(filenames[i]);
			string save_folder_final = save_folder + "/" + to_string(folder_count);
			Sleep(1500);
			// ����+����
			string save_file = CameraEngineer::Get(this->cap)->captureSaveMutex(save_folder_final, img_end);
			Sleep(1500);
		}
		//imgFile << "------" << endl;
		function_project(filenames[0]);
		cout << "Finish!" << endl;
		// �������¼��
		CameraEngineer::Get(this->cap)->start_detect();
	}

	void slot_calib() {
		cout << "please use python script to calibrate the camera" << endl;
		/*�򿪱궨���ļ�
		QString file = QFileDialog::getOpenFileName(
			this,
			QString::fromLocal8Bit("��ѡ��txtͼƬ�ļ�"),
			QDir::currentPath() + "/srcs_calib_camera/data",
			tr("Text files(*.txt)"));
		if (file.isEmpty())
		{
			return;
		}
		 ��ΪҪ���ģ���㣬�Ͽ��������
		CameraEngineer::Get(this->cap)->disconnect();
		mutex.lock();
		 ��ȡ���궨
		calibrator.calib(file.toStdString());
		mutex.unlock();
		 ��������
		slot_camera_connect();*/
	}

	// ��������
	void slot_config() {
		QString file = QFileDialog::getOpenFileName(
			this,
			QString::fromLocal8Bit("please to select a config file"),
			QDir::currentPath() + "/srcs_calib_camera/data",
			tr("Text files(*.xml)"));
		if (file.isEmpty()){
			return;
		}
		string filename = file.toStdString();

		// ���������ļ�
		calibrator.load_config(filename);
		
		CameraEngineer::Get(this->cap)->setCalibrator(calibrator);
		CameraEngineer::Get(this->cap)->start_detect();
	}

	void slot_visual() {
		// 01 ��ȡ
		QString file = QFileDialog::getOpenFileName(
			this,
			QString::fromLocal8Bit("Select a pose file"),
			QDir::currentPath() + "/srcs_calc_pose/out",
			tr("Text files(*.txt)"));
		if (file.isEmpty())
		{
			return;
		}
		string filename = file.toStdString();
		CameraEngineer::Get(this->cap)->load_nextpose(filename);
	}
};
