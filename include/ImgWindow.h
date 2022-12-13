/* ͼ�񴰿ڻ����� */

#pragma once
#include <iostream>
#include <QWidget>
#include <QImage>
#include <Qpoint>
#include <QPainter>
#include <QPaintEvent>
#include <QOpenGLWidget>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class ImgWindow:public QOpenGLWidget
{
	Q_OBJECT

protected:
	QImage qimg;
	Mat des;

public:
	ImgWindow(QWidget *parent = Q_NULLPTR) : QOpenGLWidget(parent) {};

	void paintEvent(QPaintEvent *e) {
		QPainter p(this);
		p.drawImage(QPoint(0, 0), qimg);
	};

public slots:

	/* ���Ӹú�������ʾ��ͼ���� */
	void imshow_slot(Mat img) {
		// TODO ���ͼ��仯������
		switch (img.type())
		{
		case CV_8UC1: {
			// ���ͷֱ���
			cv::resize(img, des, Size(width(), height()));
			qimg = QImage(des.data, des.cols, des.rows, des.step, QImage::Format_Grayscale8);
			break;
		}
		case CV_8UC3: {
			// ���ͷֱ���
			cv::resize(img, des, Size(width(), height()));
			qimg = QImage(des.data, des.cols, des.rows, des.step, QImage::Format_RGB888);
			break;
		default:
			cout << "û����ȷ����" << endl;
			break;
		}
		}
		update(); // �����paintEvent����
	}
};

