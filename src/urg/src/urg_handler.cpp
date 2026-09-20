#include <getopt.h>
#include <math.h>
#include <scip2awd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ssm.hpp>
#include <stdexcept>

#include "urg.hpp"

int gShutOff = 0;

// シグナルハンドラ
void ctrlC(int aStatus) {
  signal(SIGINT, NULL);
  gShutOff = 1;
}
// Ctrl+cをキャッチするための設定
void setSigInt() {
  struct sigaction sig;
  memset(&sig, 0, sizeof(sig));
  sig.sa_handler = ctrlC;
  sigaction(SIGINT, &sig, NULL);
}
int printShortHelp(const char *programName) {
  fputs("HELP\n", stderr);
  fprintf(stderr, "\t$ %s DEVICE_PATH [   options    ]\n", programName);
  fprintf(stderr, "\t$ %s -d 192.168.0.10:10940  Use URG on <IP:Port>\n", programName);
  fprintf(stderr, "\t$ %s -d /dev/ttyACM*        Use URG on </dev/ttyACM*>\n", programName);
  fprintf(stderr, "\t$ %s -d /dev/ttyACM* -n 0   Out SSM on <sensor_id=0>\n", programName);
  fputs("OPTION\n", stderr);
  fputs("\t-d | --device              : set device port\n", stderr);
  fputs("\t-i | --intensity           : Using Intensity data\n", stderr);
  fputs("\t-n | --number       NUMBRE : set sensor ID number\n", stderr);
  fputs("\t-t | --s_time       TIME   : Sampling time (defautl=40ms)\n", stderr);
  return EXIT_SUCCESS;
}

int main(int aArgc, char **aArgv) {
  // device valiant
  S2Port *port;     // URG port
  S2Param_t param;  // URG param info
  S2Sdd_t buf;      // URG multi buffer
  S2Scan_t *data;   // URG scan data buffer

  int ret;
  int i;
  double rad;
  double urg_time = 0;
  unsigned int dT = 40;  // 40ms

  char *pt;

  int sensor_id = 0;
  bool flag_intensity = false;
  bool flag_ethernet = false;

  int opt, optIndex = 0;
  struct option longOpt[] = {{"device", 1, 0, 'd'},    {"number", 1, 0, 'n'}, {"s_time", 1, 0, 't'},
                             {"intensity", 1, 0, 'i'}, {"help", 0, 0, 'h'},   {0, 0, 0, 0}};

  while ((opt = getopt_long(aArgc, aArgv, "d:n:t:ih", longOpt, &optIndex)) != -1) {
    switch (opt) {
      case 'd':
        // Open the port
        if ((pt = strchr(optarg, ':')) != NULL) {
          char *address, *port_number;
          address = strtok(optarg, ":");
          port_number = strtok(NULL, ":");
          port = Scip2_OpenEthernet(address, atoi(port_number));
          if (port != NULL) flag_ethernet = 1;
        }
        if (!flag_ethernet) port = Scip2_Open(optarg, B0);

        if (port == 0) {
          fprintf(stderr, "ERROR: Failed to open device.\n");
          return EXIT_FAILURE;
        }
        break;
      case 'n':
        sensor_id = atoi(optarg);
        break;
      case 't':
        dT = atoi(optarg);
        break;
      case 'i':
        flag_intensity = true;
        break;
      case 'h':
        printShortHelp(aArgv[0]);
        return EXIT_FAILURE;
        break;
      default:
        fprintf(stderr, "help : %s -h\n", aArgv[0]);
        return EXIT_FAILURE;
        break;
    }
  }
  // get URG parameter
  Scip2CMD_PP(port, &param);
  // Initialize buffer before getting scanned data
  S2Sdd_Init(&buf);
  // MS command
  if (flag_intensity)
    Scip2CMD_StartMS(port, param.step_min, param.step_max, 1, 0, 0, &buf, SCIP2_ENC_3X2BYTE);
  else
    Scip2CMD_StartMS(port, param.step_min, param.step_max, 1, 0, 0, &buf, SCIP2_ENC_3BYTE);

  SSMApi<urg_fs, S2Param_t> urg_fs(URG_SNAME, sensor_id);

  try {
    std::cerr << "initializing ssm ... ";
    if (!initSSM())
      throw std::runtime_error("[\033[1m\033[31mERROR\033[30m\033[0m]:fail to initialize ssm.");
    else
      std::cerr << "OK.\n";

    if (!urg_fs.create(1, (double)dT / 1000.0))
      throw std::runtime_error(
          "[\033[1m\033[31mERROR\033[30m\033[0m]:fail to create com_msg on ssm.\n");

    strncpy(urg_fs.property.model, param.model, SCIP2_MAX_LENGTH);
    urg_fs.property.dist_min = param.dist_min;
    urg_fs.property.dist_max = param.dist_max;
    urg_fs.property.step_resolution = param.step_resolution;
    urg_fs.property.step_min = param.step_min;
    urg_fs.property.step_max = param.step_max;
    urg_fs.property.step_front = param.step_front;
    urg_fs.property.revolution = param.revolution;

    printf("%s\n\n", urg_fs.property.model);

    if (!urg_fs.setProperty())
      throw std::runtime_error("[\033[1m\033[31mERROR\033[30m\033[0m]:fail to ssm open.");

    setSigInt();

    puts("start");
    while (!gShutOff) {
      // get urg data
      ret = S2Sdd_Begin(&buf, &data);
      urg_time = gettimeSSM();
      if (ret > 0) {
        // ---- analyze data ----
        for (i = 0; i < URG_DATA_MAX; i++)
          urg_fs.data.x[i] = urg_fs.data.y[i] = urg_fs.data.intensity[i] = 0;

        if (flag_intensity) {
          for (i = 0; i < data->size; i += 2) { //data->size 1080
            rad = (i / 2 - param.step_front + param.step_min) * 2.0 * M_PI / param.step_resolution;
            // Attention!!  URG data unit is [mm]
            if (rad > M_PI / 2.0 || rad < -M_PI / 2.0) continue;
            urg_fs.data.x[i/2] = (int)(data->data[i] * cos(rad));
            urg_fs.data.y[i/2] = (int)(data->data[i] * sin(rad));
            urg_fs.data.intensity[i/2] = data->data[i + 1];
          }
        } else {
          for (i = 0; i < data->size; i++) { //data->size 1080
            rad = (i - param.step_front + param.step_min) * 2.0 * M_PI / param.step_resolution;
            // Attention!!  URG data unit is [mm]
            if (rad > M_PI / 2.0 || rad < -M_PI / 2.0) continue;
            urg_fs.data.x[i] = (int)(data->data[i] * cos(rad));
            urg_fs.data.y[i] = (int)(data->data[i] * sin(rad));
            urg_fs.data.intensity[i] = 0;
          }
        }
        urg_fs.write(urg_time);
        S2Sdd_End(&buf);
      } else if (ret == -1) {
        fprintf(stderr, "ERROR: Fatal error occurred.\n");
      }
      usleep(dT * 1000);
    }
    printf("\nend\n");

    urg_fs.release();
  } catch (std::runtime_error const &error) {
    std::cout << error.what() << std::endl;
  } catch (...) {
    std::cout << "An unknown fatal error has occured. Aborting." << std::endl;
  }

  endSSM();

  return EXIT_SUCCESS;
}
