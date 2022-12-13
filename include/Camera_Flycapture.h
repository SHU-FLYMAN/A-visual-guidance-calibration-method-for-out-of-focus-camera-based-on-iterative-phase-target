/* �ҵ����Ӳ�������� */

#pragma once
#include <iostream>
#include <FlyCapture2.h>
#include <opencv2/opencv.hpp>
#include <windows.h>
#include <QThread>

using namespace std;
using namespace FlyCapture2;


class Camera_Flycapture
{
public:
	bool  is_connect = false;
	float shutter   = 0.;      // ����ع�ʱ��
	float gain	    = 0.;      // ����ϵ��

private:
	Camera cap;                // �������
	unsigned int  cam_num;     // �������
	unsigned int  id;		   // ������
	PGRGuid       guid;        // Ӳ�����
	BusManager    busMgr;      // �������
	Image		  imgRAW;	   // ԭʼ
	Image		  imgBGR;	   // ת�� 
	cv::Mat		  imgBGRMat;   // ͼ��
	unsigned int  rowByters;   // ����

	CameraInfo    cameraInfo;    // �����Ϣ
	TriggerMode   triggerMode;   // ����ģʽ
	TriggerDelay  triggerDelay;  // ����ʱ��
	FlyCapture2::Error	error;   // �洢����

public:
	// �Զ��Ͽ��������
	~Camera_Flycapture() {
		disconnect();
	};

	// �������
	bool connect(unsigned int id) {
		this->id = id;
		// ��⵽��������������0�����ҳɹ�����
		error = busMgr.GetNumOfCameras(&cam_num); is_ok(error);
		if (cam_num < id)
		{
			cerr << "Total camera num:" << cam_num << endl;
		}
		error = busMgr.GetCameraFromIndex(id, &guid); is_ok(error);
		error = cap.Connect(&guid); is_ok(error);
		error = cap.RestoreFromMemoryChannel(1); is_ok(error);
		// ��������
		setTriggeerSoft();
		// ����������
		startCaptureSoft();
		is_connect = true;
		// ��ӡ�����Ϣ
		printCameraInfo();
	
		return is_connect;
	};

	// �Ͽ�����
	bool disconnect() {
		if (is_connect)
		{
			error = cap.StopCapture();
			error = cap.Disconnect();
			if (is_ok(error))
			{
				cout << "Successfully to disconnect camera" << endl;
				is_connect = false;
			}
			else
			{
				cerr << "Failed to disconnect camera" << endl;
				is_connect = true;
			}
		}
		return is_connect;
	};

	// �����ع�
	bool setShutter(float ms = 100.) {
		if (!is_connect)
		{
			cout << "the camera has never been connected!" << endl;
			return false;
		}
		// �ع��֧����鷶Χ
		assert(ms > 0 && ms < 1000);

		Property cameraProperty;                       //�������������Ķ���
		cameraProperty.type = SHUTTER;            //���ò��������ͣ�SHUTTER�����ع�
		error = cap.GetProperty(&cameraProperty);//��ȡ��ǰ�ع�Ĳ���
		is_ok(error);

		float a = cameraProperty.absValue;

		// ���������ع�Ĳ���
		cameraProperty.absValue = ms; //���������ع�Ĵ�С����λΪms
		cameraProperty.absControl = true; //���þ���ֵ����
		cameraProperty.onOff = true; //ʹshutter������Ч
		cameraProperty.autoManualMode = false; //�ر��Զ�ģʽ���ĳ��ֶ�����ģʽ�ع��ֵ�Ż�̶�
		error = cap.SetProperty(&cameraProperty);//�����ع�

		float b = cameraProperty.absValue;
		cout << "set exp=" << b << endl;
		cv::waitKey(10); // �ȴ�������Ч

		Property sets(AUTO_EXPOSURE);
		error = cap.GetProperty(&sets);
		sets.absControl = true;
		sets.absValue = 0.;
		sets.onePush = false;
		sets.onOff = true;
		sets.autoManualMode = false;
		error = cap.SetProperty(&sets);
		cv::waitKey(100); // �ȴ�������Ч

		Property frame;
		frame.type = FRAME_RATE;
		frame.onOff = false;
		frame.autoManualMode = false;
		error = cap.SetProperty(&frame);
		cv::waitKey(10); // �ȴ�������Ч

		return is_ok(error);
	};

	bool startCaptureSoft() {
		error = cap.StartCapture(); is_ok(error);
		return is_ok(error);
	};

	// ��������
	bool setGain(float X = 0) {
		if (!is_connect)
		{
			cout << "the camera has never been connected!" << endl;
			return false;
		}
		assert(X >= 0); // ����������0
		gain = X;
		Property sets(GAIN);
		error = cap.GetProperty(&sets);
		sets.absValue = X;
		sets.absControl = true;
		sets.autoManualMode = false;
		sets.onePush = false;
		sets.onOff = false;
		sets.present = true;
		error = cap.SetProperty(&sets);
		cout << "Set gain=" << X << endl;
		cv::waitKey(50);
		return is_ok(error);
	};

	// ���ղ����浽ͼƬ�У�û��+������˲���ֱ���ã�
	bool capture(cv::Mat &img) {
		if (is_connect)
		{
			cap.RetrieveBuffer(&imgRAW);
			// ��ʱ��Ҫ��BGRת��Ϊ�Ҷ�ͼ
			imgRAW.Convert(PIXEL_FORMAT_BGR, &imgBGR);
			rowByters = (double)imgBGR.GetReceivedDataSize() / imgBGR.GetRows();
			imgBGRMat = cv::Mat(imgBGR.GetRows(), imgBGR.GetCols(), CV_8UC3, imgBGR.GetData(), rowByters);
			if (imgBGRMat.empty())
			{
				cerr << "failed to capture image!" << endl;
				return false;
			}
			else {
				cv::cvtColor(imgBGRMat, img, CV_BGR2GRAY);
				return true;
			}
		}
		else
		{
			return false;
		}
	};

	// ��ӡ�����Ϣ
	void printCameraInfo() {
		if (is_connect)
		{
			error = cap.GetCameraInfo(&cameraInfo);

			cout << "\n ############# Camera Information ############# \n" << endl;
			cout << "Num:" << cam_num << endl;
			cout << "ID:\t" << id << endl;
			cout << "Model: " << cameraInfo.modelName << endl;
			cout << "W X H:" << cameraInfo.sensorResolution << endl;
			cout << "\n ############# ----- ############# \n" << endl;
		}
		else {
			cerr << "the camera has never been connected" << endl;
		}
	};
	
	bool setTriggeerSoft() {
		error = cap.GetTriggerMode(&triggerMode); is_ok(error);
		triggerMode.onOff = false;
		triggerMode.mode = 0;
		triggerMode.parameter = 0;
		triggerMode.source = 0;
		triggerMode.polarity = 1;
		error = cap.SetTriggerMode(&triggerMode); is_ok(error);
		cv::waitKey(100);
		cout << "Trigger-Soft" << endl;
		return is_ok(error);
	};

private:
	// ������
	bool is_ok(FlyCapture2::Error &error) {
		if (error == PGRERROR_OK) {
			return true;
		}
		else {
			error.PrintErrorTrace();
			return false;
		}
	};
};
