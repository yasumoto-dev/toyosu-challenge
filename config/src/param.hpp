// パラメータファイルの読み込みと表示

#ifndef PARAM_HPP
#define PARAM_HPP

#include "config.hpp"

bool loadParam( const char *path, config_property *cnf );
void printParam( const config_property *cnf );

#endif // PARAM_HPP
