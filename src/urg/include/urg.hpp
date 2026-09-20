#ifndef URG_HPP
#define URG_HPP

#define URG_SNAME "urg_fs"
#define URG_DATA_MAX 1440

struct urg_fs {
    int x[ URG_DATA_MAX ];
    int y[ URG_DATA_MAX ];
    unsigned intensity[ URG_DATA_MAX ];
};

#endif // URG_HPP
