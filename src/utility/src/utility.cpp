#include <math.h>
#include "utility.hpp"

double trans_q(double theta)
{
  while(theta > M_PI){
    theta -= 2.0 * M_PI;
  }
  while(theta < -M_PI){
    theta += 2.0 * M_PI;
  }
  return theta;
}
