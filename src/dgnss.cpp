#include "dgnss.h"

std::string DGNSS::CAL_STATEtoString(CAL_STATE value) {
        switch (value) {
            case SOL_COMPUTED:
                return "SOL_COMPUTED";
            case INSUFFICIENT_OBS:
                return "INSUFFICIENT_OBS";
            case NO_CONVERGENCE:
                return "NO_CONVERGENCE";
            case SINGULARITY:
                return "SINGULARITY";
            case COV_TRACE:
                return "COV_TRACE";
            case TEST_DIST:
                return "TEST_DIST";
            case COLD_START:
                return "COLD_START";
            case V_H_LIMIT:
                return "V_H_LIMIT";
            case VARIANCE:
                return "VARIANCE";
            case RESIDUALS:
                return "RESIDUALS";
            case INTEGTITY_WARNING:
                return "INTEGTITY_WARNING";
            default:
                return "CAL_UNKNOWN";
        }
}

std::string DGNSS::POS_TYPEtoString(POS_TYPE value) {
        switch (value) {
            case NONE:
                return "NONE";
            case FIXEDPOS:
                return "FIXEDPOS";
            case SINGLE:
                return "SINGLE";
            case PSRDIFF:
                return "PSRDIFF";
            case NARROW_FLOAT:
                return "NARROW_FLOAT";
            case WIDE_INT:
                return "WIDE_INT";
            case SUPER_WIDE_LINE:
                return "SUPER WIDE_LINE";
            default:
                return "POS_UNKNOWN";
        }
    }

unsigned int DGNSS::calculateChecksum(const std::string& data) {
    // XOR 异或校验
    unsigned int checksum = 0;
    for (char c : data) {
        if (c != '$' && c != '*') {
            checksum ^= c;
        }
    }
    return checksum;
}

// uint8_t calculateXOR(const std::string& data) {
//     uint8_t xorValue = 0;
//     size_t startIndex = data.find_first_of("$");
//     size_t endIndex = data.find('*');

// }

bool DGNSS::verifyChecksum(const std::string& data) {
    size_t startIndex = data.find_first_of("$");
    size_t endIndex = data.find('*');

    if (startIndex != std::string::npos && endIndex != std::string::npos) {
        std::string checksumStr = data.substr(endIndex + 1, 2);
        try {
            unsigned int receivedChecksum = std::stoi(checksumStr, nullptr, 16);
            std::string message = data.substr(startIndex + 1, endIndex - startIndex - 1);
            unsigned int calculatedChecksum = calculateChecksum(message);
            return receivedChecksum == calculatedChecksum;
        } catch (const std::exception& e) {}
    }
    return false;
}

unsigned long DGNSS::calculateCRC32(const char *szBuf, int iSize) {
    int iIndex;
    unsigned long ulCRC = 0;
    for (iIndex=0; iIndex<iSize; iIndex++) {
        ulCRC = aulCrcTable[(ulCRC ^ szBuf[iIndex]) & 0xff] ^ (ulCRC >> 8);
    }
    return ulCRC;
}

class InvalidCRCException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Invalid CRC exception";
    }
};

bool DGNSS::verify32bitCRC (const std::string& headingdata){
    size_t startIndex = headingdata.find_first_of("#");
    size_t endIndex = headingdata.find('*');
    unsigned long receivedCRC;
    std::string crcStr;
    std::stringstream ss;
    if (startIndex != std::string::npos && endIndex != std::string::npos) {
        // std::cout << "========= Enter 32bit CRC Chcek" << std::endl;
        crcStr = headingdata.substr(endIndex + 1, 8);
        try {
            // std::cout << crcStr << std::endl;
            // receivedCRC = std::stoi(crcStr, nullptr, 16);
            // std::cout << receivedCRC << std::endl;
            std::string message = headingdata.substr(startIndex + 1, endIndex - startIndex - 1);
            // std::cout << "~~~~~~~~~~ Enter Try Statement" << std::endl;
            const char* data = message.c_str();
            unsigned long calculatedCRC = calculateCRC32(data, strlen(data));
            // return receivedCRC == calculatedCRC;
            ss << std::hex << calculatedCRC;
            return crcStr == ss.str();
        // } catch (const std::exception& e) {}
        } catch (const std::invalid_argument& e) {
            std::cout << "Invalid argument exception: " << e.what() << std::endl;
        } catch (const InvalidCRCException& e) {
            std::cout << "Invalid CRC exception: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Standard exception: " << e.what() << std::endl;
        }
    }
    std::cout << "---------- Without Enter 32bit CRC Chcek ----------" << std::endl;
    return false;
    
}

double DGNSS::convertDeg(const std::string& degree) {
    double deg = 0;
    size_t minIndex = degree.find('.') - 2;
    std::string degStr = degree.substr(0,minIndex);
    std::string minStr = degree.substr(minIndex);
    deg = std::stod(degStr) + std::stod(minStr)/60;
    return deg;
}

bool DGNSS::data_extract(const std::string& data, std::chrono::high_resolution_clock::time_point& now) {
    
    // add system sample time stamp
    auto sys_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    
    // data type judge
    DATA_TYPE data_type;
    if (data[0] == '$') {
        if(data[1] == 'G') {
            data_type = GNRMC;
        } else if(data[1] == 'K') {
            data_type = KSXT;
        }
    } else if (data[0] == '#') {
        if (data[1] == 'H') {
            data_type = HEADINGA;
        } else if(data[1] == 'B') {
            data_type = BESTPOSA;
        }
    } else {
        return false;
    }
    std::vector<std::string> temp;

    switch (data_type)
    {
    case GNRMC: {
        // verify check sum
        if (verifyChecksum(data)) {
            // std::cout << "Checksum valided" << std::endl;
            gnrmc_packet_.data_valid = true;
        } else {
            //std::cout << "Checksum not valided" << std::endl;
            gnrmc_packet_.data_valid = false;
        }

        size_t start = 0, end;
        while((end = data.find(',', start)) != std::string::npos ) {
            temp.push_back(data.substr(start, end - start));
            start = end + 1;
        }

        // save to struct
        gnrmc_packet_.gnss_format = temp[0];
        gnrmc_packet_.gnss_time = temp[1];
        gnrmc_packet_.sys_timestamp_microsec = static_cast<std::int64_t>(sys_timestamp);
        if (temp[2] == "A") {
            gnrmc_packet_.postiton_valid = true;
        } else {
            gnrmc_packet_.postiton_valid = false;
        }
        gnrmc_packet_.latitiude = convertDeg(temp[3]);
        gnrmc_packet_.latitude_NS = *temp[4].data();
        gnrmc_packet_.longtitude = convertDeg(temp[5]);
        gnrmc_packet_.longitude_EW = *temp[6].data();
        gnrmc_packet_.ground_velocity = std::stof(temp[7]);
        gnrmc_packet_.ground_heading = std::stof(temp[8]);
        gnrmc_packet_.UTC_date = std::stoul(temp[9]);
        gnrmc_packet_.magnetic_declination =  std::stof(temp[10]);
        gnrmc_packet_.magnetic_declination_EW = *temp[11].data();
        read_GNRMC = true;
        return gnrmc_packet_.data_valid;
        break;
    }

    case KSXT: {
        if(verifyChecksum(data)) {
            ksxt_packet_.data_valid = true;
        } else {
            ksxt_packet_.data_valid = false;
        }

        size_t start_heading = 0, end_heading;
        while((end_heading = data.find(',', start_heading)) != std::string::npos ) {
            temp.push_back(data.substr(start_heading, end_heading - start_heading));
            start_heading = end_heading + 1;
        }

        // save to struct
        ksxt_packet_.gnss_format = temp[0];
        ksxt_packet_.utctime = temp[1];
        ksxt_packet_.sys_timestamp_microsec = static_cast<std::int64_t>(sys_timestamp);
        ksxt_packet_.longitude = std::stod(temp[2]);
        ksxt_packet_.latitude = std::stod(temp[3]);
        ksxt_packet_.height = std::stod(temp[4]);
        ksxt_packet_.heading = std::stof(temp[5]);
        ksxt_packet_.pitch = std::stof(temp[6]);
        ksxt_packet_.angular_velocity = std::stof(temp[7]);
        ksxt_packet_.vel_horizontal = std::stof(temp[8]);
        if(!temp[9].empty()){
            ksxt_packet_.roll = std::stof(temp[9]);
        } else{
            ksxt_packet_.roll = 0;
        }
        
        ksxt_packet_.pos_quality = std::stoi(temp[10]);
        ksxt_packet_.heading_quality = std::stoi(temp[11]);
        ksxt_packet_.satelliteNum_mainAntenna = std::stoi(temp[12]);
        ksxt_packet_.satelliteNum_slaveAntenna = std::stoi(temp[13]);
        if(!temp[14].empty()){
            ksxt_packet_.pos_e = std::stof(temp[14]);
        } else{
            ksxt_packet_.pos_e = 0;
        }
        if(!temp[14].empty()){
            ksxt_packet_.pos_n = std::stof(temp[15]);
        } else{
            ksxt_packet_.pos_n = 0;
        }
        if(!temp[14].empty()){
            ksxt_packet_.pos_u = std::stof(temp[16]);
        } else{
            ksxt_packet_.pos_u = 0;
        }
        ksxt_packet_.vel_e = std::stof(temp[17]);
        ksxt_packet_.vel_n = std::stof(temp[18]);
        ksxt_packet_.vel_u = std::stof(temp[19]);
        
        read_KSXT = true;
        return ksxt_packet_.data_valid;
        break;
    }

    case HEADINGA: {

        // verify 32bit CRC
        if (verify32bitCRC(data)) {
            heading_packet_.data_valid = true;
        } else {
            heading_packet_.data_valid = false;
        }

        size_t start_heading = 0, end_heading;
        while((end_heading = data.find(',', start_heading)) != std::string::npos ) {
            temp.push_back(data.substr(start_heading, end_heading - start_heading));
            start_heading = end_heading + 1;
        }

        // save to struct
        heading_packet_.gnss_format = temp[0];
        heading_packet_.sys_timestamp_microsec = static_cast<std::int64_t>(sys_timestamp);
        // enum CAL_STATE{SOL_COMPUTED, INSUFFICIENT_OBS, COLD_START};
        std::string temp2 = temp[9];
        std::string cal_state = temp2.substr(temp2.find(';',0)+1);
        if (cal_state == CAL_STATEtoString(SOL_COMPUTED)) {
            heading_packet_.cal_state = SOL_COMPUTED;
        } else if (cal_state == CAL_STATEtoString(INSUFFICIENT_OBS)) {
            heading_packet_.cal_state = INSUFFICIENT_OBS;
        } else if (cal_state == CAL_STATEtoString(COLD_START)) {
            heading_packet_.cal_state = COLD_START;
        } else {
            heading_packet_.cal_state = CAL_UNKNOWN;
        }

        // enum POS_TYPE{NONE, FIXEDPOS, SINGLE, PSRDIFF, NARROW_FLOAT, WIDE_INT, SUPER_WIDE_LINE, POS_UNKNOWN};
        if (temp[10] == POS_TYPEtoString(NONE)) {
            heading_packet_.post_type = NONE;
        } else if (temp[10] == POS_TYPEtoString(FIXEDPOS)) {
            heading_packet_.post_type = FIXEDPOS;
        } else if (temp[10] == POS_TYPEtoString(SINGLE)) {
            heading_packet_.post_type = SINGLE;
        } else if (temp[10] == POS_TYPEtoString(PSRDIFF)) {
            heading_packet_.post_type = PSRDIFF;
        } else if (temp[10] == POS_TYPEtoString(NARROW_FLOAT)) {
            heading_packet_.post_type = NARROW_FLOAT;
        } else if (temp[10] == POS_TYPEtoString(WIDE_INT)) {
            heading_packet_.post_type = WIDE_INT;
        } else if (temp[10] == POS_TYPEtoString(SUPER_WIDE_LINE)) {
            heading_packet_.post_type = SUPER_WIDE_LINE;
        } else {
            heading_packet_.post_type = POS_UNKNOWN;
        }

        heading_packet_.baseline_length = std::stof(temp[11]);
        heading_packet_.heading_angle = std::stof(temp[12]);
        heading_packet_.pitch_angle = std::stof(temp[13]);
        heading_packet_.heading_std = std::stof(temp[15]);
        heading_packet_.pitch_std = std::stof(temp[16]);
        read_HEADINGA = true;
        return heading_packet_.data_valid;
        break;
    }

    case BESTPOSA: {

        // verify 32 bit CRC
        if(verify32bitCRC(data)) {
            bestposa_packet_.data_valid = true;
        } else {
            bestposa_packet_.data_valid = false;
        }

        // divided into partical
        size_t start_heading = 0, end_heading;
        while((end_heading = data.find(',',start_heading)) != std::string::npos) {
            temp.push_back(data.substr(start_heading, end_heading - start_heading));
            start_heading = end_heading + 1;
        }

        // save to struct
        bestposa_packet_.gnss_format = temp[0];
        
        bestposa_packet_.sys_timestamp_microsec = static_cast<std::int64_t>(sys_timestamp);
        
        std::string temp2 = temp[9];
        std::string cal_state = temp2.substr(temp2.find(';',0)+1);
        if (cal_state == CAL_STATEtoString(SOL_COMPUTED)) {
            bestposa_packet_.cal_state = SOL_COMPUTED;
        } else if (cal_state == CAL_STATEtoString(INSUFFICIENT_OBS)) {
            bestposa_packet_.cal_state = INSUFFICIENT_OBS;
        } else if (cal_state == CAL_STATEtoString(NO_CONVERGENCE)) {
            bestposa_packet_.cal_state = NO_CONVERGENCE;
        } else if (cal_state == CAL_STATEtoString(SINGULARITY)) {
            bestposa_packet_.cal_state = SINGULARITY;
        } else if (cal_state == CAL_STATEtoString(COV_TRACE)) {
            bestposa_packet_.cal_state = COV_TRACE;
        } else if (cal_state == CAL_STATEtoString(TEST_DIST)) {
            bestposa_packet_.cal_state = TEST_DIST;
        } else if (cal_state == CAL_STATEtoString(COLD_START)) {
            bestposa_packet_.cal_state = COLD_START;
        } else if (cal_state == CAL_STATEtoString(V_H_LIMIT)) {
            bestposa_packet_.cal_state = V_H_LIMIT;
        } else if (cal_state == CAL_STATEtoString(VARIANCE)) {
            bestposa_packet_.cal_state = VARIANCE;
        } else if (cal_state == CAL_STATEtoString(RESIDUALS)) {
            bestposa_packet_.cal_state = RESIDUALS;
        } else if (cal_state == CAL_STATEtoString(INTEGTITY_WARNING)) {
            bestposa_packet_.cal_state = INTEGTITY_WARNING;
        } else{
            bestposa_packet_.cal_state = CAL_UNKNOWN;
        }

        // enum POS_TYPE{NONE, FIXEDPOS, SINGLE, PSRDIFF, NARROW_FLOAT, WIDE_INT, SUPER_WIDE_LINE, POS_UNKNOWN};
        if (temp[10] == POS_TYPEtoString(NONE)) {
            bestposa_packet_.pos_type = NONE;
        } else if (temp[10] == POS_TYPEtoString(FIXEDPOS)) {
            bestposa_packet_.pos_type = FIXEDPOS;
        } else if (temp[10] == POS_TYPEtoString(SINGLE)) {
            bestposa_packet_.pos_type = SINGLE;
        } else if (temp[10] == POS_TYPEtoString(PSRDIFF)) {
            bestposa_packet_.pos_type = PSRDIFF;
        } else if (temp[10] == POS_TYPEtoString(NARROW_FLOAT)) {
            bestposa_packet_.pos_type = NARROW_FLOAT;
        } else if (temp[10] == POS_TYPEtoString(WIDE_INT)) {
            bestposa_packet_.pos_type = WIDE_INT;
        } else if (temp[10] == POS_TYPEtoString(SUPER_WIDE_LINE)) {
            bestposa_packet_.pos_type = SUPER_WIDE_LINE;
        } else {
            bestposa_packet_.pos_type = POS_UNKNOWN;
        }

        bestposa_packet_.latitude = std::stod(temp[11]);
        bestposa_packet_.longtitude = std::stod(temp[12]);
        bestposa_packet_.height = std::stod(temp[13]);
        
        bestposa_packet_.undulation = std::stof(temp[14]);
        
        bestposa_packet_.latitude_sigma = std::stof(temp[16]);
        bestposa_packet_.longitude_sigma = std::stof(temp[17]);
        bestposa_packet_.height_sigma = std::stof(temp[18]);

        read_BESTPOSA = true;
        return bestposa_packet_.data_valid;
        break;
    }

    default:
        return false;
        break;
    }

    return false;
}

double DGNSS::calcGNRMC_HEADING_timestampdiff() {
    // judge data exist;
    if (!read_GNRMC || !read_HEADINGA) {
        throw std::runtime_error("GNRMC or HEADINGA are not read!");
        exit(-1);
    }
    std::int64_t timestamp_diff;
    timestamp_diff = gnrmc_packet_.sys_timestamp_microsec - heading_packet_.sys_timestamp_microsec;
    double diff = (double)(long double)timestamp_diff;
    diff = diff * 10e-8;
    return diff;
}


void DGNSS::printInfo() {
    //GNRMC
    if (read_GNRMC) {
        std::cout << "Dataformat: " << gnrmc_packet_.gnss_format << std::endl;
        std::cout << "UTC Time: " << gnrmc_packet_.gnss_time << std::endl;
        std::cout << "Sys Time(microsec): " << gnrmc_packet_.sys_timestamp_microsec << std::endl;
        std::cout << "Position Valid: " << gnrmc_packet_.postiton_valid << std::endl;
        std::cout << "Latitude: " << std::fixed << std::setw(14) << std::setprecision(8) << std::left
            << gnrmc_packet_.latitiude << gnrmc_packet_.latitude_NS << std::endl;
        std::cout << "Longtitude: " << std::fixed << std::setw(14) << std::setprecision(8) << std::left
            << gnrmc_packet_.longtitude << gnrmc_packet_.longitude_EW << std::endl;
        std::cout << "Ground Velovity: " << gnrmc_packet_.ground_velocity << std::endl;
        std::cout << "Ground Heading: " << gnrmc_packet_.ground_heading << std::endl;
        std::cout << "UCT Date: " << gnrmc_packet_.UTC_date << std::endl;
        std::cout << "Magnetic Declination: " << gnrmc_packet_.magnetic_declination << gnrmc_packet_.magnetic_declination_EW << std::endl;
        std::cout << "Data Valid: " << gnrmc_packet_.data_valid << std::endl;
    }
    
    // HEADINGA
    if (read_HEADINGA) {
        std::cout << "Dataformat: " << heading_packet_.gnss_format << std::endl;
        std::cout << "Sys Time(microsec): " << heading_packet_.sys_timestamp_microsec << std::endl;
        std::cout << "Calculated State: " << CAL_STATEtoString(heading_packet_.cal_state) << std:: endl;
        std::cout << " Position Type: " << POS_TYPEtoString(heading_packet_.post_type) << std::endl;
        std::cout << "Baseline Length: " << heading_packet_.baseline_length << std::endl;
        std::cout << "Heading Angle: " << heading_packet_.heading_angle << std::endl;
        std::cout << "Heading Angle STD: " << heading_packet_.heading_std << std::endl;
        std::cout << "Pitch Angle: " << heading_packet_.pitch_angle << std::endl;
        std::cout << "Pitch Angle STD:" << heading_packet_.pitch_std << std::endl;
        std::cout << "Data Valid: " << heading_packet_.data_valid << std::endl;

    }   

    // BESTPOSA
    if (read_BESTPOSA) {
        std::cout << "Dataformat: " << bestposa_packet_.gnss_format << std::endl;
        std::cout << "Sys Time(microsec): " << bestposa_packet_.sys_timestamp_microsec << std::endl;
        std::cout << "Calculated State: " << CAL_STATEtoString(bestposa_packet_.cal_state) << std:: endl;
        std::cout << " Position Type: " << POS_TYPEtoString(bestposa_packet_.pos_type) << std::endl;
        std::cout << "Latitude: " << bestposa_packet_.latitude << std::endl;
        std::cout << "Latitude STD: " << bestposa_packet_.latitude_sigma << std::endl;
        std::cout << "Longitude: " << bestposa_packet_.longtitude << std::endl;
        std::cout << "Longitude STD: " << bestposa_packet_.longitude_sigma << std::endl;
        std::cout << "Undulation: " << bestposa_packet_.undulation << std::endl;
        std::cout << "Height: " << bestposa_packet_.height << std::endl;
        std::cout << "Height STD: " << bestposa_packet_.height_sigma << std::endl;
        std::cout << "Data Valid: " << bestposa_packet_.data_valid << std::endl;
    }
}    

bool DGNSS::print2File(std::ofstream& fid) {
    
    // if (gnrmc_packet_.data_valid) {
    //     fid << std::setw(20) << std::left << gnrmc_packet_.sys_timestamp_microsec;
    //     fid << std::setw(14) << std::left << gnrmc_packet_.gnss_time;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << gnrmc_packet_.latitiude;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << gnrmc_packet_.longtitude;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << gnrmc_packet_.ground_heading;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << gnrmc_packet_.ground_velocity;

    // } else {
    //     fid << std::setw(20) << std::left << gnrmc_packet_.sys_timestamp_microsec;
    //     fid << std::setw(14) << std::left << gnrmc_packet_.gnss_time;
    //     fid << std::setw(20) << std::left << "DATA VALID FALSE" << std::setw(20) << " ";
    // }

    // if (heading_packet_.data_valid) {
    //     fid << std::setw(20) << std::left << heading_packet_.sys_timestamp_microsec;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << heading_packet_.baseline_length;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << heading_packet_.heading_angle;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << heading_packet_.heading_std;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << heading_packet_.pitch_angle;
    //     fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << heading_packet_.pitch_std;

    // } else {
    //     fid << std::setw(20) << std::left << heading_packet_.sys_timestamp_microsec;
    //     fid << std::setw(12) << std::left << "DATA VALID FALSE" << std::setw(12) << " ";
    // }
    // gnrmc
    fid << std::setw(20) << std::left << gnrmc_packet_.sys_timestamp_microsec;
    fid << std::setw(14) << std::left << gnrmc_packet_.gnss_time;
    
    // fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << gnrmc_packet_.latitiude;
    // fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << gnrmc_packet_.longtitude;
    fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << bestposa_packet_.latitude;
    fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << bestposa_packet_.longtitude;
    // 大地高 = 海拔高 + 椭球与水准面高程
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << bestposa_packet_.height + bestposa_packet_.undulation;
    
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << bestposa_packet_.latitude_sigma;
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << bestposa_packet_.longitude_sigma;
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << bestposa_packet_.height_sigma;

    // heading
    // fid << std::setw(20) << std::left << heading_packet_.sys_timestamp_microsec;
    
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << heading_packet_.baseline_length;
    
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << heading_packet_.pitch_angle;
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << heading_packet_.heading_angle;
    
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << heading_packet_.pitch_std;
    fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << heading_packet_.heading_std;
    
    fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << gnrmc_packet_.ground_velocity;
    fid << std::fixed << std::setprecision(8) << std::setw(14) << std::left << gnrmc_packet_.ground_heading;

    // bestposa
    // fid << std::setw(20) << std::left << bestposa_packet_.sys_timestamp_microsec;
    
    // fid << std::fixed << std::setprecision(4) << std::setw(14) << std::left << bestposa_packet_.undulation;
    
    fid << std::setw(4) << std::left << gnrmc_packet_.data_valid;
    fid << std::setw(4) << std::left << heading_packet_.data_valid;
    fid << std::setw(4) << std::left << bestposa_packet_.data_valid;

    fid << std::endl;
    fid.flush();
    return true;
}



