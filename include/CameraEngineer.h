/* ���������װ��������Ⱦ������ */

#pragma once
#include <iostream>
#include <direct.h>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QDir>
#include <opencv2/opencv.hpp>
#include "Camera_Flycapture.h"
#include "Calibrator.h"



using namespace std;
using namespace cv;


class CameraEngineer:public QThread
{
	Q_OBJECT
public:
	Calibrator *calibrator;  // ���ͼ��ǵ�
	bool stop_flag = false;  // ��¼�Ƿ�ֹͣ�����ⲿ�޸ģ�
	bool is_detect = false;  // �Ƿ�ʵʱ���
	bool is_guide = false;   // �Ƿ���̬����
	vector<cv::Point2f> pts2d_nextpose;

protected:
	int img_count = 0;       // ���ռ���
	QMutex mutex1, mutex2;   // ��
	Camera_Flycapture *cam;  // �������
	Mat img, img_save;                 // ���ڱ���ͼ��
	// ����ģʽ�����ҽ���һ��ʵ������
	CameraEngineer(Camera_Flycapture &cam) {
		this->cam = &cam;
		// �����̣߳�����run
		start();
	}; // ���������治�������ɶ�����

public:
	static CameraEngineer *Get(Camera_Flycapture &cam) {
		static CameraEngineer imgThread(cam);
		return &imgThread;
	}

	// ʵʱ�������
	void run() {
		int wait_time = 20;
		// ������
		while (true)
		{
			// ������û�����ӣ���ô��100ms���ٴγ���
			// stop_flag �����ж����������Ƿ�ֹͣ
			if (stop_flag)
			{
				return;
			}
			if (!cam->is_connect)
			{
				msleep(100);
				continue;
			}
			// ���is_connect���������ˣ����ǻ���Ҫ�˳�
			mutex1.lock();
			if (stop_flag)
			{
				return;
			}
			// ����
			cam->capture(img);
				
			// ���ǵ㣬��ʾ��ͼ����
			
			if (is_detect)
			{
				// ��ı�ͼ���С
				this->calibrator->detectCornersFast(img, 0.3);
			}
			// �Ƿ���ʾ�����ǵ�
			if (is_guide)
			{
				this->calibrator->show_nextpose(img, pts2d_nextpose);
			}

			// �������
			if (is_detect && is_guide) {
				// ������ߵľ��룬������ʾ��ͼ����
				this->calibrator->calc_distance(img, pts2d_nextpose);
			}
			
			// ����ͼ��
			img_signal(img);
			mutex1.unlock();
			msleep(wait_time);
		}
	};

	// ���ղ�����ͼƬ����Ҫ+���������ĵ�һ��ûд�룩
	string captureSaveMutex(const string &folder="./out", const string &img_end=".bmp") {
		// �½��ļ���
		mkdir(folder);
		mutex2.lock();
		cam->capture(img_save);
		img_count += 1;
		string filename = folder + "/" + to_string(img_count) + img_end;
		cv::imwrite(filename, img_save);
		cout << "save image into:" << filename << endl;
		mutex2.unlock();
		return filename;
	};

	void disconnect() {
		mutex1.lock();
		cam->disconnect();
		mutex1.unlock();
	}

	void setCalibrator(Calibrator &calibrator) {
		this->calibrator = &calibrator;
	}

	void start_detect() {
		mutex2.lock();
		is_detect = true;
		mutex2.unlock();
	}

	void stop_detect() {
		mutex2.lock();
		is_detect = false;
		mutex2.unlock();
	}

	void setImageCountZeros() {
		img_count = 0;
	}

	void mkdir(const string &folder) {
		QString folder_qstr = QString::fromStdString(folder);
		QDir dir;
		// ����ļ��в�����
		if (!dir.exists(folder_qstr))
		{
			dir.mkpath(folder_qstr);
		}
	}


	void load_nextpose(const string &filename) {
		is_guide = this->calibrator->load_nextpose(filename, pts2d_nextpose);
	}


signals:
	// �źţ����ڷ���ͼƬ
	void img_signal(Mat img);
};

