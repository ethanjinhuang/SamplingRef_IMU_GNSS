#include "zimu.h"

uint8_t ZIMU::calChecksum(void *pStart, uint32_t uSize, uint32_t Init, uint32_t Xor)
{
    uint8_t uCRCValue;
    uint8_t *pData = static_cast<uint8_t*>(pStart); // Explicitly cast
    uCRCValue = Init;
    // Your checksum calculation logic goes here
    while (uSize--)
    {
        uCRCValue ^= *pData++;
    }
    return uCRCValue ^ Xor;
}

bool ZIMU::verifyChecksum(const std::string &data)
{
    const char* p_buff = data.c_str();

    // 查找"*"的位置
    const char* p_asterisk = std::strstr(p_buff, "*");

    // 如果没有找到"*"，或者"*"的位置在字符串结尾之前，返回 false
    if (p_asterisk == nullptr || p_asterisk >= p_buff + data.size() - 2)
    {
        return false;
    }

    // 解析接收到的校验和，hex转换为整数
    char* endptr;
    unsigned long receivedChecksum = std::strtoul(p_asterisk + 1, &endptr, 16);
    // if (*endptr != '\0' || endptr >= p_buff + data.size() - 2)
    // {
    //     return false;
    // }

    // 计算校验和的范围，从第二个字符开始到"*"之前
    uint32_t cs_len = p_asterisk - (p_buff + 1);

    // 调用 calChecksum 计算校验和
    uint8_t calculatedChecksum = calChecksum(const_cast<char*>(p_buff + 1), cs_len, 0, 0);

    
    // 返回是否匹配
    return calculatedChecksum == receivedChecksum;
}

// bool ZIMU::data_extract(const std::string &data, std::chrono::high_resolution_clock::time_point &now)
// {
//     // convert to system time stamp
//     auto sys_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

//     // determain data type 
//     DATA_TYPE data_type;
//     std::string dataheader = data.substr(0,6);
//     if (dataheader == "$GHFPD") {
//         data_type = GHFPD;
//     } else {
//         return false;
//     }

//     std::vector<std::string> temp;

//     switch (data_type)
//     {
//         case GHFPD:
//         {
//             // verify checksum
//             if(verifyChecksum(data)){
//                 ghfpd_packet_.data_valid = true;
//             } else {
//                 ghfpd_packet_.data_valid = false;
//             }
//             // data slice
//             size_t start = 0, end;
//             while ((end = data.find(',', start)) != std::string::npos || 
//                 (end = data.find('*', start)) != std::string::npos) {
//                 temp.push_back(data.substr(start, end - start));
//                 start = end + 1;
//             }
//             // save to data struct
//             ghfpd_packet_.packet_format = temp[0];
//             ghfpd_packet_.sys_timestamp_microsec = static_cast<std::int64_t>(sys_timestamp);
//             ghfpd_packet_.gps_week = std::stoi(temp[1]);
//             ghfpd_packet_.gps_time = std::stod(temp[2]);
//             ghfpd_packet_.heading = std::stof(temp[3]);
//             ghfpd_packet_.pitch = std::stof(temp[4]);
//             ghfpd_packet_.roll = std::stof(temp[5]);
//             ghfpd_packet_.latitude = std::stod(temp[6]);
//             ghfpd_packet_.longitude = std::stod(temp[7]);
//             ghfpd_packet_.altitude = std::stof(temp[8]);
//             ghfpd_packet_.vn = std::stof(temp[9]);
//             ghfpd_packet_.ve = std::stof(temp[10]);
//             ghfpd_packet_.vd = std::stof(temp[11]);
//             ghfpd_packet_.baseline = std::stof(temp[12]);
//             ghfpd_packet_.nsv1 = std::stoi(temp[13]);
//             ghfpd_packet_.nsv2 = std::stoi(temp[14]);
//             ghfpd_packet_.status = std::stoi(temp[15]);
//             read_GHFPD = true;
            
//             return ghfpd_packet_.data_valid;
//             break;
//         }
//         default:
//             return false;
//             break;
//     }
// }

bool ZIMU::data_extract(const std::string &data, std::chrono::high_resolution_clock::time_point &now) {
    // Convert to system time stamp
    auto sys_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

    // Determine data type
    if (data.substr(0, 6) != "$GHFPD") {
        return false; // Unsupported data type
    }

    // Verify checksum
    if (!verifyChecksum(data)) {
        return false; // Invalid checksum
    }

    // Split the data (',' and '*' as delimiters)
    std::vector<std::string> temp;
    size_t start = 0, end;
    while ((end = data.find_first_of(",*", start)) != std::string::npos) {
        temp.push_back(data.substr(start, end - start));
        start = end + 1;
    }
    temp.push_back(data.substr(start)); // Push the last segment

    // Validate minimum fields
    if (temp.size() < 16) { // New format has at least 16 fields
        return false;
    }

    try {
        // Save data to the struct
        ghfpd_packet_.data_valid = true;
        ghfpd_packet_.packet_format = temp[0]; // "$GHFPD"
        ghfpd_packet_.sys_timestamp_microsec = static_cast<std::int64_t>(sys_timestamp);

        // Data fields
        ghfpd_packet_.gps_week = std::stoi(temp[1]); // Fixed value '0', skip further validation
        ghfpd_packet_.gps_time = std::stod(temp[2]);
        ghfpd_packet_.heading = std::stof(temp[3]);
        ghfpd_packet_.pitch = std::stof(temp[4]);
        ghfpd_packet_.roll = std::stof(temp[5]);
        ghfpd_packet_.latitude = std::stod(temp[6]);
        ghfpd_packet_.longitude = std::stod(temp[7]);
        ghfpd_packet_.altitude = std::stof(temp[8]);
        ghfpd_packet_.vn = std::stof(temp[9]);
        ghfpd_packet_.ve = std::stof(temp[10]);
        ghfpd_packet_.vd = std::stof(temp[11]);
        ghfpd_packet_.baseline = std::stof(temp[12]);

        // Queue sizes (optional fields)
        ghfpd_packet_.nsv1 = std::stoi(temp[13]); // Default 0 if isMoniterSensorQueue_ is false
        ghfpd_packet_.nsv2 = std::stoi(temp[14]);

        // Navigation status
        ghfpd_packet_.status = std::stoi(temp[15]);

        read_GHFPD = true;
        return ghfpd_packet_.data_valid;

    } catch (const std::exception &e) {
        // Handle parsing errors
        ghfpd_packet_.data_valid = false;
        return false;
    }
}