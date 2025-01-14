#include "serial_driver.h"
#include "dgnss.h"
#include "zimu.h"
#include "ColorParse.h"
#include <iomanip>
#include <signal.h>
#include <thread>
#include <iostream>

using namespace std;

SerialDriver serial_GNSS; 
SerialDriver serial_IMU; 

std::ofstream outputfile;
DGNSS dgnss;
ZIMU zimu;

bool is_GNSS_new = false;
bool is_IMU_new = false;
bool is_Send_Init = false;

void thread_load_serial_GNSS();
void thread_load_serial_IMU();
void thread_save2file();
void thread_sendINITcommand();
unsigned int CRC32Checksum(const std::string &data);

int main(int argc, char* argv[]){
// int main(){
    std::string serial_name_GNSS;
    std::string baudrate_GNSS;

	std::string serial_name_IMU;
    std::string baudrate_IMU;
    
	// serial_name_IMU = "/dev/ttyCH9344USB3";
	// baudrate_IMU = "115200";
	// serial_name_GNSS = "/dev/ttyCH9344USB1";
	// baudrate_GNSS = "115200";
	// std::cout << termColor("magenta") << "Using Default Parameter: " << std::endl;
	// std::cout << termColor("magenta") << "IMU: " << serial_name_IMU << ", " << baudrate_IMU << std::endl;
	// std::cout << termColor("magenta") << "GNSS: " << serial_name_GNSS << ", " << baudrate_GNSS << std::endl;


	if (argc<5) {

        serial_name_IMU = "/dev/ttyUSB0";
		baudrate_IMU = "115200";
		serial_name_GNSS = "/dev/ttyUSB1";
        baudrate_GNSS = "115200";

		std::cout << termColor("magenta") << "Using Default Parameter: " << std::endl;
		std::cout << termColor("magenta") << "IMU: " << serial_name_IMU << ", " << baudrate_IMU << std::endl;
		std::cout << termColor("magenta") << "GNSS: " << serial_name_GNSS << ", " << baudrate_GNSS << std::endl;
    } else {
        serial_name_IMU = argv[1];
        baudrate_IMU = argv[2];
		serial_name_GNSS = argv[3];
        baudrate_GNSS = argv[4];
		std::cout << termColor("magenta") << "Using Parameter: " << std::endl;
		std::cout << termColor("magenta") << "IMU: " << serial_name_IMU << ", " << baudrate_IMU << std::endl;
		std::cout << termColor("magenta") << "GNSS: " << serial_name_GNSS << ", " << baudrate_GNSS << std::endl;
    }
	std::cout << termColor("white");
	// serial_name_GNSS = "/dev/ttyCH9344USB1";
	// serial_name_IMU = "/dev/ttyCH9344USB3";
	// baudrate_GNSS = "115200";
	// baudrate_IMU = "115200";



    serial_GNSS.open(serial_name_GNSS,baudrate_GNSS);
    serial_GNSS.flush();
	serial_IMU.open(serial_name_IMU,baudrate_IMU);
	serial_IMU.flush();


    // current time stamp
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    // convert time stamp to local time
    std::tm* local_time = std::localtime(&now_c);

    // output file
	char filename[100];
    std::strftime(filename, sizeof(filename), "ZIMU_GNSS_%Y%m%d_%H%M%S.txt", local_time);

    outputfile.open(filename,std::ios::out|std::ios::app);
    
	if(!outputfile.is_open()) {
        exit(-1);
    }


	thread thread_GNSS(thread_load_serial_GNSS);
	thread thread_IMU(thread_load_serial_IMU);
	thread thread_Save2file(thread_save2file);
	thread threat_Sendinitcommand(thread_sendINITcommand);
	thread_GNSS.join();
	thread_IMU.join();
	thread_Save2file.join();
	threat_Sendinitcommand.join();

	return 0;
}

void thread_load_serial_GNSS(){
	uint8_t temp;
    std::string tempdata;
	bool findHeader = false;
    bool findCRLF = false;
	bool serialState = false;
	std::chrono::high_resolution_clock::time_point doller_sec;

	while(1) {
		tempdata.clear();
		
		while (!findHeader || !findCRLF) {
			serialState = serial_GNSS.readByte(temp);
			if(serialState) {
				// find '$'/'#' or '\r'
				if (temp == '$' || temp == '#') {
					doller_sec = std::chrono::high_resolution_clock::now();
					findHeader = true;
				} else if (temp == '\r') {
					findCRLF = true;
				}
				
				if (findHeader && !findCRLF) {
					tempdata += temp;
				}
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		findHeader = false;
		findCRLF = false;

		// finished one packet of data
		if(tempdata[0] == '$' && tempdata[1] == 'G') {
			std::cout << termColor("lightgreen") << tempdata << std::endl;
		} else if(tempdata[0] == '$' && tempdata[1] == 'K') {
			std::cout << termColor("lightyellow") << tempdata << std::endl;
		} else if(tempdata[0] == '#' && tempdata[1] == 'H') {
			std::cout << termColor("lightblue") << tempdata << std::endl;
		} else if(tempdata[0] == '#' && tempdata[1] == 'B') {
			std::cout << termColor("lightcyan") << tempdata << std::endl;
		} else {
			std::cout << termColor("white") << tempdata << std::endl;
		}

		dgnss.data_extract(tempdata, doller_sec);
		
		
		
		if(dgnss.read_GNRMC && dgnss.read_HEADINGA && dgnss.read_BESTPOSA && dgnss.read_KSXT) {
			is_GNSS_new = true;
		} else {
			continue;
		}
		dgnss.read_BESTPOSA = false;
		dgnss.read_GNRMC = false;
		dgnss.read_HEADINGA = false;
		dgnss.read_KSXT = false;
		// std::cout << "TEST GNSS" << std::endl;
		// std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}



void thread_load_serial_IMU(){
	uint8_t temp;
	std::string tempdata;
	bool findHeader = false;
    bool findCRLF = false;
	bool serialState = false;
	std::chrono::high_resolution_clock::time_point doller_sec;
	while(1) {
		tempdata.clear();
		while(!findHeader || !findCRLF) {
			serialState = serial_IMU.readByte(temp);
			if(serialState) {
				// find $
				if (temp == '$') {
					doller_sec = std::chrono::high_resolution_clock::now();
					findHeader = true;
				} else if (temp == '\r') {
					findCRLF = true;
				}

				if (findHeader && !findCRLF) {
					tempdata += temp;
				}
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		findHeader = false;
		findCRLF = false;

		std::cout << termColor("lightred") << tempdata << std::endl;
		zimu.data_extract(tempdata, doller_sec);
		if(zimu.read_GHFPD) {
			is_IMU_new = true;
		} else {
			continue;
		}

		zimu.read_GHFPD = false;
		
		
		// std::cout << "TEST IMU" << std::endl;
		// std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	
}

void thread_save2file(){
	while(1) {
		
		if(is_IMU_new) {
			if(is_GNSS_new) {
				// 同时写入新的IMU和新的GNSS数据
				outputfile << std::setw(20) << std::left << zimu.ghfpd_packet_.sys_timestamp_microsec;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.gps_time;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << zimu.ghfpd_packet_.latitude;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << zimu.ghfpd_packet_.longitude;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << zimu.ghfpd_packet_.altitude;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.vn;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.ve;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.vd;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.roll;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.pitch;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.heading;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.status;

				outputfile << std::setw(4) << std::left << 1;	// 是否有新GNSS数据标志位
				outputfile << std::setw(20) << std::left << dgnss.gnrmc_packet_.sys_timestamp_microsec;
				outputfile << std::setw(10) << std::left << (dgnss.gnrmc_packet_.sys_timestamp_microsec - zimu.ghfpd_packet_.sys_timestamp_microsec)*1e-6;
				outputfile << std::setw(20) << std::left << dgnss.gnrmc_packet_.gnss_time;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << dgnss.bestposa_packet_.latitude;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << dgnss.bestposa_packet_.longtitude;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.bestposa_packet_.height + dgnss.bestposa_packet_.undulation;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.height;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.bestposa_packet_.latitude_sigma;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.bestposa_packet_.longitude_sigma;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.bestposa_packet_.height_sigma;

				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.baseline_length;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.pitch_angle;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.heading_angle;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.pitch_std;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.heading_std;

				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.gnrmc_packet_.ground_velocity;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.gnrmc_packet_.ground_heading;

				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.vel_n/3.6;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.vel_e/3.6;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << - dgnss.ksxt_packet_.vel_u/3.6;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.roll;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.pitch;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.heading;
				
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.angular_velocity;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.vel_horizontal;

				if(is_Send_Init){
					outputfile << std::setw(4) << std::left << zimu.ghfpd_packet_.data_valid + 10;
					is_Send_Init = false;
				} else{
					outputfile << std::setw(4) << std::left << zimu.ghfpd_packet_.data_valid;
				}

				outputfile << std::setw(4) << std::left << dgnss.gnrmc_packet_.data_valid;
				outputfile << std::setw(4) << std::left << dgnss.heading_packet_.data_valid;
				outputfile << std::setw(4) << std::left << dgnss.bestposa_packet_.data_valid;
				outputfile << std::setw(4) << std::left << dgnss.ksxt_packet_.data_valid;

				outputfile << std::endl;

				outputfile.flush();
				is_IMU_new = false;
				is_GNSS_new = false;
			} else {
				// 只写入新的IMU数据和旧的GNSS数据
				outputfile << std::setw(20) << std::left << zimu.ghfpd_packet_.sys_timestamp_microsec;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.gps_time;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << zimu.ghfpd_packet_.latitude;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << zimu.ghfpd_packet_.longitude;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << zimu.ghfpd_packet_.altitude;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.vn;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.ve;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.vd;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.roll;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.pitch;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.heading;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << zimu.ghfpd_packet_.status;

				outputfile << std::setw(4) << std::left << 0;	// 是否有新GNSS数据标志位
				outputfile << std::setw(20) << std::left << dgnss.gnrmc_packet_.sys_timestamp_microsec;
				outputfile << std::setw(10) << std::left << (dgnss.gnrmc_packet_.sys_timestamp_microsec - zimu.ghfpd_packet_.sys_timestamp_microsec)*1e-6;
				outputfile << std::setw(20) << std::left << dgnss.gnrmc_packet_.gnss_time;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << dgnss.bestposa_packet_.latitude;
				outputfile << std::fixed << std::setprecision(8) << std::setw(14) << std::left << dgnss.bestposa_packet_.longtitude;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.bestposa_packet_.height + dgnss.bestposa_packet_.undulation;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.height;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.bestposa_packet_.latitude_sigma;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.bestposa_packet_.longitude_sigma;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.bestposa_packet_.height_sigma;

				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.baseline_length;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.pitch_angle;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.heading_angle;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.pitch_std;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.heading_packet_.heading_std;

				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.gnrmc_packet_.ground_velocity;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.gnrmc_packet_.ground_heading;

				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.vel_n/3.6;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.vel_e/3.6;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << - dgnss.ksxt_packet_.vel_u/3.6;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.roll;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.pitch;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.heading;
				
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.angular_velocity;
				outputfile << std::fixed << std::setprecision(4) << std::setw(14) << std::left << dgnss.ksxt_packet_.vel_horizontal;


				if(is_Send_Init){
					outputfile << std::setw(4) << std::left << zimu.ghfpd_packet_.data_valid + 10;
					is_Send_Init = false;
				} else{
					outputfile << std::setw(4) << std::left << zimu.ghfpd_packet_.data_valid;
				}
				
				outputfile << std::setw(4) << std::left << dgnss.gnrmc_packet_.data_valid;
				outputfile << std::setw(4) << std::left << dgnss.heading_packet_.data_valid;
				outputfile << std::setw(4) << std::left << dgnss.bestposa_packet_.data_valid;
				outputfile << std::setw(4) << std::left << dgnss.ksxt_packet_.data_valid;

				outputfile << std::endl;
				
				outputfile.flush();
				is_IMU_new = false;
				continue;
			}
		} else {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
		
		
		
		// outputfile << "Test" << std::endl;
		// outputfile.flush();
		// // outputfile << "TEST SAVE TO FILE" << std::endl;
		// std::cout << termColor("lightgreen") << outputfile.is_open() << std::endl;
		// std::this_thread::sleep_for(std::chrono::seconds(1));

	}
}


void thread_sendINITcommand(){
	std::this_thread::sleep_for(std::chrono::seconds(10));
	while (1) {
		if (is_GNSS_new) {
			double latitude = dgnss.bestposa_packet_.latitude;
			double longitude = dgnss.bestposa_packet_.longtitude;
			double height = dgnss.bestposa_packet_.height + dgnss.bestposa_packet_.undulation;
			std::string initcommand = "$INITPOS,";
			std::ostringstream oss;
			oss << "$INITPOS," << std::fixed << std::setprecision(8) << latitude;
			oss << ",N,"<< std::fixed << std::setprecision(8) << longitude << ",E,";
			oss << std::fixed << std::setprecision(4) << height << "*";
			unsigned int crcres = CRC32Checksum(oss.str());
			oss << std::hex << crcres << "\r\n";
			initcommand = oss.str();
			std::cout << termColor("lightblue") << initcommand << std::endl;
			// print to serial (double send to make sure imu system received)
			for(u_int8_t c:initcommand) {
				serial_IMU.writeByte(c);
			}
			for(u_int8_t c:initcommand) {
				serial_IMU.writeByte(c);
			}
			serial_IMU.flush();
			std::cout << termColor("green") << "SUCCESSFUL SEND INIT COMMAND: ";
			std::cout << termColor("lightblue") << initcommand << std::endl;

			is_Send_Init = true;
			break;
		}	
	}
	
	// double latitude = dgnss.bestposa_packet_.latitude;
	// double longitude = dgnss.bestposa_packet_.longtitude;
	// double height = dgnss.bestposa_packet_.height + dgnss.bestposa_packet_.undulation;
	// double latitude = 29.99927505;
	// double longitude = 125.16195925;
	// double height = 11.5151;
	// std::string initcommand = "$INITPOS,";
	// std::ostringstream oss;
	// oss << "$INITPOS," << std::fixed << std::setprecision(8) << latitude;
	// oss << ",N,"<< std::fixed << std::setprecision(8) << longitude << ",E,";
	// oss << std::fixed << std::setprecision(4) << height << "*";
	// unsigned int crcres = CRC32Checksum(oss.str());
	// oss << std::hex << crcres << "\r\n";
	// initcommand = oss.str();
	// for(u_int8_t c:initcommand) {
	// 	// std::cout << c << std::endl;
	// 	serial_IMU.writeByte(c);
		
	// }
	// serial_IMU.flush();
	// std::cout << termColor("green") << "SUCCESSFUL SEND INIT COMMAND: ";
	// std::cout << termColor("lightblue") << initcommand << std::endl;
}


unsigned int CRC32Checksum(const std::string &data) {
    unsigned int checksum = 0;
    for (char c : data) {
		if(c == '*') {
				break;
		}
        if (c != '$' && c!= '*') {
            checksum ^= c;
        }
    }
    return checksum;
}