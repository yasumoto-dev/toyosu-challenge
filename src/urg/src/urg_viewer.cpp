#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <ssmtype/spur-odometry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ssm.hpp>
#include <stdexcept>

#include "urg.hpp"
#include <opencv2/opencv.hpp>

int gShutOff = 0;

static const int W = 600, H = 600;
static const double SCALE = 50.0;
static const int DISPLAY_W = 600, DISPLAY_H = 600;

static void ctrlC( int aStatus );
static void setSigInt( ); 
static void printShortHelp( const char *programName );
static cv::Point toPixel( double x, double y );
static void drawGrid( cv::Mat &img );
static cv::Scalar intensityToColor( unsigned intensity );


int main(int aArgc, char *aArgv[]) {
  int i;
  int sensor_id = 0;
  bool flag_intensity = false;
  bool flag_laser = false;
  bool flag_spur = false;
  FILE *gnuplot;
  double distance;
  unsigned int dT = 40;  // 40ms

  int opt, optIndex = 0;
  struct option longOpt[] = {{"number", 1, 0, 'n'}, {"s_time", 1, 0, 't'}, {"intensity", 1, 0, 'i'},
                             {"laser", 1, 0, 'l'}, {"help", 0, 0, 'h'}, {0, 0, 0, 0}};

  while ((opt = getopt_long(aArgc, aArgv, "n:t:ilsh", longOpt, &optIndex)) != -1) {
    switch (opt) {
      case 'n':
        sensor_id = atoi(optarg);
        break;
      case 't':
        dT = atoi(optarg);
        break;
      case 'i':
        flag_intensity = true;
        break;
      case 'l':
        flag_laser = true;
        break;
      case 'h':
        printShortHelp(aArgv[0]);
        return 1;
        break;
      default:
        fprintf(stderr, "help : %s -h\n", aArgv[0]);
        return 1;
        break;
    }
  }

  SSMApi<urg_fs> urg_fs(URG_SNAME, sensor_id);

  try {
    std::cerr << "initializing ssm ... ";
    if (!initSSM())
      throw std::runtime_error("[\033[1m\033[31mERROR\033[30m\033[0m]:fail to initialize ssm.");
    else
      std::cerr << "OK.\n";

    std::cerr << "open urg ... ";
    if (!urg_fs.open(SSM_READ))
      throw std::runtime_error("[\033[1m\033[31mERROR\033[30m\033[0m]:fail to open urg on ssm.\n");
    else
      std::cerr << "OK.\n";
 
    setSigInt();

    cv::namedWindow("urg_viewer", cv::WINDOW_NORMAL);
    cv::resizeWindow("urg_viewer", DISPLAY_W, DISPLAY_H);

    cv::Mat img(H, W, CV_8UC3);

    while (!gShutOff) {
      if (urg_fs.readNew()) {
        img = cv::Scalar(255, 255, 255);
        drawGrid(img);

        // -lオプション (レーザー線)
        if(flag_laser) {
          for (i = 0;i < URG_DATA_MAX; i++) {
            distance = sqrt(pow(urg_fs.data.x[i] / 1000.0, 2) + pow(urg_fs.data.y[i] / 1000.0, 2));
            if (distance > 30 || distance < 0.1) continue;
            cv::line(img, toPixel(0, 0), toPixel(urg_fs.data.x[i] / 1000.0, urg_fs.data.y[i] / 1000.0),
                                          cv::Scalar(0, 200, 0), 1);
          }
        }

        // 点群描画
        for (i = 0;i < URG_DATA_MAX; i++) {
          if (urg_fs.data.x[i] == 0 && i > 1) continue;
          distance = sqrt(pow(urg_fs.data.x[i] / 1000.0, 2) + pow(urg_fs.data.y[i] / 1000.0,2));
          if (distance > 30 || distance < 0.1) continue;

          cv::Point p = toPixel(urg_fs.data.x[i] / 1000.0, urg_fs.data.y[i] / 1000.0); 
          cv::Scalar color = flag_intensity ? intensityToColor(urg_fs.data.intensity[i])
                                              : cv::Scalar(0, 0, 255);
          // 点を表示
          cv::circle(img, p, 1, color, -1);
        }

        cv::imshow("urg_viewer", img);
        cv::waitKey(1);
      } else {
        usleep(dT * 1000);
      }
    }

    cv::destroyAllWindows();

    urg_fs.release();
    printf("\nend\n");

  } catch (std::runtime_error const &error) {
    std::cout << error.what() << std::endl;
  } catch (...) {
    std::cout << "An unknown fatal error has occured. Aborting." << std::endl;
  }

  endSSM();

  return 0;
}


static void ctrlC(int aStatus) {
  signal(SIGINT, NULL);
  gShutOff = 1;
}

static void setSigInt() {
  struct sigaction sig;
  memset(&sig, 0, sizeof(sig));
  sig.sa_handler = ctrlC;
  sigaction(SIGINT, &sig, NULL);
}

static void printShortHelp(const char *programName) {
  fputs("HELP\n", stderr);
  fprintf(stderr, "\t$ %s [   options    ]\n", programName);
  fprintf(stderr, "\t$ %s -i -l -s -n 0\n", programName);
  fputs("OPTION\n", stderr);
  fputs("\t-i | --intensity           : Usin Intensity data\n", stderr);
  fputs("\t-l | --laser               : Draw laser\n", stderr);
  fputs("\t-s | --spur                : using spur odometry\n", stderr);
  fputs("\t-n | --number       NUMBRE : Set sensor ID number\n", stderr);
  fputs("\t-t | --s_time       TIME   : Sampling time (defautl=40ms)\n", stderr);
}

// 直行座標をpixelに変換
static cv::Point toPixel(double x, double y) {
  int px = W / 2 + (int)(x * SCALE);
  int py = H / 2 - (int)(y * SCALE);

  return cv::Point(px, py);
}

// grid線と軸ラベルを追加
static void drawGrid(cv::Mat &img) {
  int grid = 6;
  for (int m = -grid; m <= grid; m++) {
    cv::line(img, toPixel(m, -grid), toPixel(m, grid), cv::Scalar(220, 220, 220), 1);
    cv::line(img, toPixel(-grid, m), toPixel(grid, m), cv::Scalar(220, 220, 220), 1);
  }
  cv::line(img, toPixel(-grid, 0), toPixel(grid, 0), cv::Scalar(100, 100, 100), 1);
  cv::line(img, toPixel(0, -grid), toPixel(0, grid), cv::Scalar(100, 100, 100), 1);

  // 軸のラベル
  for (int m = -grid; m <= grid; m += 2) {
    if (m == 0) continue;

    // x軸(前方方向)のラベル
    cv::Point px_label = toPixel(m, 0);
    cv::putText(img, std::to_string(m), cv::Point(px_label.x - 8, px_label.y + 18),
                cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar(100, 100, 100), 1, cv::LINE_AA);

    // y軸(左右方向)のラベル
    cv::Point py_label = toPixel(0, m);
    cv::putText(img, std::to_string(m), cv::Point(py_label.x + 6, py_label.y + 18),
                cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar(100, 100, 100), 1, cv::LINE_AA);
  }

  // 原点ラベル
  cv::putText(img, "0", cv::Point(toPixel(0, 0).x + 6, toPixel(0, 0).y + 18),
              cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar(100,100, 100), 1, cv::LINE_AA);
}

// intensity(0~6000)をJETカラーマップでBGRの色に変換
static cv::Scalar intensityToColor(unsigned intensity) {
  int v = (int)intensity;
  if (v < 0) v = 0;
  if (v > 6000) v = 6000;
  uchar normalized = (uchar)(v * 255 / 6000);

  cv::Mat src(1, 1, CV_8UC1, cv::Scalar(normalized));
  cv::Mat dst;
  cv::applyColorMap(src, dst, cv::COLORMAP_JET);
  cv::Vec3b c = dst.at<cv::Vec3b>(0, 0);

  return cv::Scalar(c[0], c[1], c[2]);
}


