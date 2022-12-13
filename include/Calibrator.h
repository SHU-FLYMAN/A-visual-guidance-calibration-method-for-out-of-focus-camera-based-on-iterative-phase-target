#pragma once

#include <iostream>
#include <math.h>
#include <stdexcept>
#include <opencv2/opencv.hpp>
using namespace std;


class Calibrator
{
public:
	// ����ȡ������
	cv::Size board_size;             // ���̸�ߴ�
	double   square_size;            // �����С��mm��
	double   square_size_pixel;      // �����С�����أ�
	// ���� fy ��Ϊ���ɲ�����fx/fy �ı��������� cameraMatrix �еı�����ͬ          
	bool     aspectRatio;            // 
	bool     calibFixPrincipalPoint; // ������̶�������
	bool     calibZerok1Dist;        // �������� k1 ����Ϊ��
	bool     calibZerok2Dist;        // �������� k2 ����Ϊ��
	bool     calibZeroTangentDist;   // �����������Ϊ��
	int      flags_calib = 0;        // �궨���
	bool     is_config;              // �Ƿ���ɲ�������
	string   output_dir;             // �궨�������ļ�
	int      temp_num = 0;
	int      N;                      // ���Ʋ���
	float    B_min;                  // ��С���ƶ�
	cv::Mat img_corners, img_circles, pha, img_scale1, img_scale2, B;             // ͼ
	double   scale;                  // ���¼��ʱ������ų߶�
	// �������
public:
	vector<cv::Point2f> pts2d_corners_fast, pts2d_corners, pts2d_circles, pts2d_pha;
	vector<vector<cv::Point2f> > pts2d_corners_all, pts2d_circles_all, pts2d_pha_all;
	vector<vector<cv::Point3f> > pts3d_world_all;
	vector<cv::Point3f> pts3d_world;
	vector<vector<double> > pts2d_nextpose_double;

	cv::Size imgSize;                                   // ͼ��ߴ�
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);  // ����ڲ�
	cv::Mat distCoeffs;                                 // ����ϵ��
	vector<cv::Mat> rvecs, tvecs;                       // ��ת��ƽ�ƾ���
	vector<float> reprojErrs;                           // �Լ��������ͶӰ���                      
	double rms, rms_avg;                                // OpenCV �������ͶӰ���
	char buf[1024];
public:
	// ��ȡ�ļ�
	void load_config(const string &file){
		cout << "load config file...:" << file << endl;
		// ��ȡ����
		cv::FileStorage fr(file, cv::FileStorage::READ);
		cv::FileNode node = fr.getFirstTopLevelNode();

		/* �궨����ز��� */
		double inch, Screen_Size, Screen_Width_Ratio, Screen_Height_Ratio,
			Screen_Width_Pixel, Screen_Height_Pixel,
			BoardSize_Width, BoardSize_Height;

		node["N"] >> N;
		node["B_min"] >> B_min;
		node["inch"] >> inch;
		node["Screen_Size"] >> Screen_Size;
		node["Screen_Width_Ratio"] >> Screen_Width_Ratio;
		node["Screen_Height_Ratio"] >> Screen_Height_Ratio;
		node["Screen_Width_Pixel"] >> Screen_Width_Pixel;
		node["Screen_Height_Pixel"] >> Screen_Height_Pixel;
		node["BoardSize_Width"] >> BoardSize_Width;
		node["BoardSize_Height"] >> BoardSize_Height;

		// ���̸��С
		board_size.width = BoardSize_Width;   // �ж�����
		board_size.height = BoardSize_Height;  // �ж�����
		// ��Ļ������
		double Screen_Width = inch * Screen_Size * (Screen_Width_Ratio / sqrt(Screen_Width_Ratio * Screen_Width_Ratio + Screen_Height_Ratio * Screen_Height_Ratio));
		
		square_size = Screen_Width / (BoardSize_Width + 3);  // ���Ӵ�С
		square_size_pixel = Screen_Width_Pixel / (BoardSize_Width + 3);

		/* �궨���ò��� */
		node["Calibrate_FixAspectRatio"] >> aspectRatio;
		node["Calibrate_AssumeZeroTangentialDistortion"] >> calibZeroTangentDist;
		node["Calibrate_FixPrincipalPointAtTheCenter"] >> calibFixPrincipalPoint;
		node["Calibrate_AssumeZerok1Distortion"] >> calibZerok1Dist;
		node["Calibrate_AssumeZerok2Distortion"] >> calibZerok2Dist;

		if (aspectRatio)            flags_calib |= CV_CALIB_FIX_ASPECT_RATIO;
		if (calibZeroTangentDist)   flags_calib |= CV_CALIB_ZERO_TANGENT_DIST;
		if (calibFixPrincipalPoint) flags_calib |= CV_CALIB_FIX_PRINCIPAL_POINT;
		if (calibZerok1Dist)        flags_calib |= CV_CALIB_FIX_K1;
		if (calibZerok2Dist)        flags_calib |= CV_CALIB_FIX_K2;

		// �ļ���·��
		node["output_dir"] >> output_dir;
	
		this->is_config = true;
		fr.release();

		cout << "Successfully load config file."<< endl;
	}

	/* ���ټ��ǵ� */
	void detectCornersFast(cv::Mat &img, double scale = 0.5) {
		if (is_config)
		{
			this->scale = scale;
			// ���ͷֱ����������������
			cv::resize(img, img_scale1, cv::Size(0, 0), scale, scale);
			// �ܴ��Եļ�⣬���λ�ûᷢ���仯
			bool found;
			try
			{
				bool chess = true;
				if (chess){
					found = cv::findChessboardCorners(img_scale1, board_size, pts2d_corners_fast,	CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FAST_CHECK);
				} 
				else
				{
					found = detectCircles(img_scale1, pts2d_corners_fast);
				}
			}
			// �����κ��쳣����ʱδ��⵽�ǵ㣩
			catch (exception* e)
			{
				return;
			}

			// ��ͼ��ת��Ϊbgr������imgͼ���СҲ������ת��,��Ϊ��һ�룩
			cv::cvtColor(img_scale1, img, cv::COLOR_GRAY2BGR);
			// �������̸�
			cv::drawChessboardCorners(img, board_size, cv::Mat(pts2d_corners_fast), found);
			cv::resize(img, img, cv::Size(0, 0), 1. / scale, 1. / scale);
		}
	}

	void detect(vector<string> &files) {
		//// 01 �ǵ�
		img_corners = cv::imread(files[0], 0);
		bool found_corners = detectCorners(img_corners, pts2d_corners);
		drawBoard(img_corners, pts2d_corners, found_corners, "corners");
		pts2d_corners_all.push_back(pts2d_corners);
		pts3d_world_all.push_back(pts3d_world);
	}

	/* ���ǵ� */
	bool detectCorners(cv::Mat &img, vector<cv::Point2f> &points_2d) {
		if (is_config)
		{
			bool found = cv::findChessboardCorners(img, board_size, points_2d, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_NORMALIZE_IMAGE);
			if (found)
			{
				// ��ϸ���ҽǵ�
				cv::cornerSubPix(img, points_2d, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			}
			return found;
		}
		else
		{
			cerr << "�궨��δ����" << endl;
			return false;
		}
		
	}

	bool detectCircles(cv::Mat &img, vector<cv::Point2f> &points_2d) {
		bool found = cv::findCirclesGrid(255 - img, board_size, points_2d);
		return found;
	}

	bool detectPhaseCircles(vector<string> &files, cv::Mat &pha, vector<cv::Point2f> &points_2d, int N) {
		// ���������λ������
		cv::Size img_size = cv::imread(files[0], 0).size();
		
		cv::Mat sin_sum = cv::Mat::zeros(img_size, CV_64FC1);
		cv::Mat cos_sum = cv::Mat::zeros(img_size, CV_64FC1);
		cv::Mat Ik;
		pha = cv::Mat::zeros(img_size, CV_64FC1);
		B = cv::Mat::zeros(img_size, CV_64FC1);
		for (int i = 2; i < files.size(); i++)
		{
			int idx = i - 2;
			Ik = cv::imread(files[i], 0);
			Ik.convertTo(Ik, CV_64FC1);
			Ik /= 255.; // ͼ��
			double p = 2 * idx / N * CV_PI ;
			sin_sum += sin(p) * Ik;
			cos_sum += cos(p) * Ik;
		}
		for (int row =0; row < Ik.rows; row++){
			for (int col = 0; col < Ik.cols; col++){
				double s = sin_sum.at<double>(row, col);
				double c = cos_sum.at<double>(row, col);
				pha.at<double>(row, col) = atan2(s, c);
				B.at<double>(row, col) = sqrt(s * s + c * c) * 2 / double(N);
			}
		}

		double e = -0.0000000001;
		cv::Mat pha_low_mask = (pha < e) / 255.;
		pha_low_mask.convertTo(pha_low_mask, CV_64FC1);
		pha = pha + pha_low_mask * 2. * CV_PI;

		// ��һ��
		pha /= (2 * CV_PI);

		cv::Mat B_mask = (B >= B_min) / 255.;
		B_mask.convertTo(B_mask, CV_64FC1);
		pha = pha.mul(B_mask);
		cv::imshow("pha", pha);
		cv::waitKey(0);
		return true;
	}

	void drawBoard(const cv::Mat &img, vector<cv::Point2f> pts2d, bool found, const string &win_name) {
		cv::cvtColor(img, img_scale2, cv::COLOR_GRAY2BGR);
		cv::drawChessboardCorners(img_scale2, board_size, cv::Mat(pts2d), found);
		cv::imshow(win_name, img_scale2);
		cv::waitKey(10);
	}

	// ������л���
	void clear() {
		rvecs.clear();
		tvecs.clear();
		pts3d_world.clear();
		pts3d_world_all.clear();
		pts2d_corners_fast.clear();
		pts2d_corners.clear();
		pts2d_circles.clear();
		pts2d_pha.clear();
		pts2d_corners_all.clear();
		pts2d_circles_all.clear();
		pts2d_pha_all.clear();
	}

	// �Դ��ı궨���ܣ�����ʹ��Python�ű�������
	void calib(const string &filename) {
		cout << "�Ƽ���Python�ű��н��б궨" << endl;
		return;

		// �����Ķ���������
		if (!is_config)
		{
			cerr << "δ���������ļ�" << endl;
			return;
		}
		clear();  // ��������ڴ�
		cout << "��ʼ���ǵ�..." << endl;
		// ��ȡ�궨��ͼƬ
		ifstream fr;
		fr.open(filename, ios_base::in);
		// ��֧������
		if (!fr.is_open())
		{
			cout << "δ�ܳɹ����ļ�:" << filename << endl;
		}
		
		// ��������ϵ
		for (int i = 0; i < board_size.height; i++){
			for (int j = 0; j < board_size.width; j++){
				pts3d_world.emplace_back(
					cv::Point3f(float(j * square_size), float(i * square_size), 0.));
			}
		}

		// ��ȡ���ļ��е�����ͼƬ·��
		vector <string> files;           // ����õ��ļ�
		string string_line;

		int idx = 1;
		while (getline(fr, string_line))
		{
			// ���ļ���ӵ�files��
			if (string_line != "------")
			{
				files.emplace_back(string_line);
				continue;
			}
			if (idx == 1) {
				// ��ȡ�궨ͼƬ��С
				imgSize = cv::imread(files[0]).size();
			}
			cout << "���:" << idx << "ͼƬ" << endl;
			// ���ǵ�
			detect(files);
			// ���¶�ȡͼƬ
			files.resize(0);
			idx += 1;
		}

		cv::destroyAllWindows();
		fr.close();

		cout << "��ʼ�궨..." << endl;
		// ��ȡ�������б궨
		if (flags_calib & CV_CALIB_FIX_ASPECT_RATIO)
		{
			cameraMatrix.at<double>(0, 0) = 1.0;
		}
		
		// ����궨����
		rms = cv::calibrateCamera(pts3d_world_all, pts2d_corners_all, imgSize, cameraMatrix, distCoeffs, rvecs, tvecs, flags_calib | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5);

		cout << "��ͶӰ���" << rms << endl;
		// ������ͶӰ���
		rms_avg = calcReprojectionErrors();

		saveCameraParas(output_dir);
		cout << "��ɱ궨" << endl;
	}

	// ������ͶӰ���
	double calcReprojectionErrors() {
		vector<cv::Point2f> pts2d_proj;   // ͶӰ��
		reprojErrs.resize(pts3d_world_all.size());
		int totalPoints = 0;   // �ܵ���
		double totalErr = 0;
		int n = (int)pts2d_corners_all.size();
		for (int idx = 0; idx < (int)pts3d_world_all.size(); ++idx)
		{
			// ��3d���������ͶӰ��2d���ص�
			cv::projectPoints(cv::Mat(pts3d_world_all[idx]), rvecs[idx], tvecs[idx], cameraMatrix, distCoeffs, pts2d_proj);
			double err = cv::norm(cv::Mat(pts2d_corners_all[idx]), cv::Mat(pts2d_proj), CV_L2);
			reprojErrs[idx] = (float)std::sqrt(err * err / n);
			totalErr += err * err;
			totalPoints += n;
		}
		return std::sqrt(totalErr / totalPoints);
	}

	void saveCameraParas(const string &output_dir) {
		string file = output_dir + "paras.xml";
		cout << "save camera paras into file:" << file << endl;
		// 01 ��¼�궨�����ò���
		cv::FileStorage fw(file, cv::FileStorage::WRITE);
		fw << "calibration_Time" << calc_time();  		// ����ʱ��
		fw << "nrOfFrames" << (int)rvecs.size();
		fw << "image_Width" << imgSize.width;
		fw << "image_Height" << imgSize.height;
		fw << "board_Width" << board_size.width;
		fw << "board_Height" << board_size.height;
		fw << "square_Size" << square_size;
		fw << "k1_Dist" << calibZerok1Dist;
		fw << "k2_Dist" << calibZerok2Dist;
		fw << "flagValue" << flags_calib;
		fw << "Camera_Matrix" << cameraMatrix;
		fw << "Distortion_Coefficients" << distCoeffs;
		fw << "Avg_Reprojection_Error" << rms_avg;
		fw.release();

		ofstream txt_cameraMatrix;  // �������
		ofstream txt_distortCoeff;  // �������
		ofstream txt_rvecs;         // ��ת����
		ofstream txt_tvecs;         // ƽ�ƾ���
		ofstream txt_pixel;         // ��������
		txt_cameraMatrix.open(output_dir + "camera_matrix.txt");
		txt_distortCoeff.open(output_dir + "distort_coeff.txt");
		txt_rvecs.open(output_dir + "rotation_matrix.txt");
		txt_tvecs.open(output_dir + "translation_vector.txt");
		txt_pixel.open(output_dir + "pixel_points.txt");

		// �ڲ�
		txt_cameraMatrix << cameraMatrix;
		// ����
		txt_distortCoeff << distCoeffs;
		// ���
		cv::Mat r_vec, t, r_mat;
		for (int i = 0; i < (int)rvecs.size(); i++){
			r_vec = rvecs[i].t();  // ����3 x 1������
			t = tvecs[i].t();
			cv::Rodrigues(r_vec, r_mat);  // ת��Ϊ 3 x 3 ���� 
			txt_rvecs << r_mat.reshape(0, 1) << endl;
			txt_tvecs << t << endl;
		}
		// ���ص�
		for (int i = 0; i < (int)pts2d_corners_all.size(); i++){
			txt_pixel << pts2d_corners_all[i] << endl;
		}
		txt_cameraMatrix.close();
		txt_distortCoeff.close();
		txt_rvecs.close();
		txt_tvecs.close();
		txt_pixel.close();
	}

	char* calc_time() {
		time_t tm;
		time(&tm);
		struct tm *t2 = localtime(&tm);
		strftime(buf, sizeof(buf) - 1, "%c", t2);
		return buf;
	}

	void show_nextpose(cv::Mat &img, vector<cv::Point2f> &pt2d_nextpose) {
		if (!is_config)
		{
			cout << "Never load config file!" << endl;
			return;
		}
		int cols = (int)board_size.width;  // �������̸������		
		cv::Scalar c = cv::Scalar(255, 0, 0);
		int w = 7;
		int line_type = 4;
		int num = pt2d_nextpose.size();

		// ���Ʊ߿�
		cv::Point2f p_1 = pt2d_nextpose[0];
		cv::Point2f p_2 = pt2d_nextpose[cols - 1];
		cv::Point2f p_3 = pt2d_nextpose[num - 1];
		cv::Point2f p_4 = pt2d_nextpose[num - cols];

		cv::line(img, p_1, p_2, c, w, line_type);
		cv::line(img, p_2, p_3, c, w, line_type);
		cv::line(img, p_3, p_4, c, w, line_type);
		cv::line(img, p_4, p_1, c, w, line_type);
	}

	bool load_nextpose(const string &filename, vector<cv::Point2f> &pt2d_nextpose) {
		cout << "��ȡλ���ļ�" << endl;
		this->pts2d_nextpose_double.clear();
		pt2d_nextpose.clear();
		ifstream infile;
		infile.open(filename);
		while (infile) {
			string s;
			if (!getline(infile, s)) break;

			istringstream ss(s);
			vector <double> record;
			while (ss)
			{
				string s;
				if (!getline(ss, s, ',')) break;
				stringstream fs(s);
				double f = 0.0;
				fs >> f;
				record.push_back(f);
			}
			pts2d_nextpose_double.push_back(record);
		}

		cv::Point2f p;
		for (int i = 0; i < pts2d_nextpose_double[0].size(); i++) {
			p.x = pts2d_nextpose_double[0][i];
			p.y = pts2d_nextpose_double[1][i];
			pt2d_nextpose.emplace_back(p);
		}
		if (!infile.eof()) {
			cerr << "Cannot find the file containing next pose!\n";
		}
		infile.close();
		return true;
	}

	void calc_distance(cv::Mat &img, vector<cv::Point2f> &pts2d_nextpose) {
		if (!is_config)
		{
			cout << "Never load config file!" << endl;
			return;
		}
		// ������
		int num = pts2d_corners_fast.size();
		if (num == 0 || pts2d_nextpose.size() == 0)
		{
			return;
		}
		double dis_mean = 0.;
		for (int i=0; i < num; i++)
		{
			cv::Point2f p1 = pts2d_nextpose[i];
			cv::Point2f p2 = pts2d_corners_fast[i] / scale;
			cv::Point2f dxy = p1 - p2;
			double d = sqrt(dxy.x * dxy.x + dxy.y * dxy.y);
			dis_mean += (d / num);
		}
		string s = cv::format("error=%.0fpixel", dis_mean);
		cv::putText(img, s.c_str(), cv::Point(50, 100), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 0, 0), 8, 0);
	}
};