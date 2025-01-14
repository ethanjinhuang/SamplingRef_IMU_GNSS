
#ifndef ZIMU_H
#define ZIMU_H

#include <string>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cstring>
#include <cstdint>
#include <sstream>

class ZIMU {
public:

    enum DATA_TYPE{GHFPD};

    bool read_GHFPD = false;

    struct GHFPD_packet {
        std::string packet_format;
        std::int64_t sys_timestamp_microsec;
        int gps_week;
        double gps_time;
        float heading;
        float pitch;
        float roll;
        double latitude;
        double longitude;
        float altitude;
        float vn;
        float ve;
        float vd;
        float baseline;
        int nsv1;
        int nsv2;
        int status;
        bool data_valid;
    }ghfpd_packet_;

    uint8_t calChecksum(void *pStart, uint32_t uSize, uint32_t Init, uint32_t Xor);

    bool verifyChecksum(const std::string& data);

    bool data_extract(const std::string & data, std::chrono::high_resolution_clock::time_point& now);
};


#endif // ZIMU_H
