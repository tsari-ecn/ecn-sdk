
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "UDPServer.h"

struct InsDataType {
    std::string header; // header, $GPCHC
    int gps_week;       // weeks from 1980-1-6
    double gps_time;    // seconds from 0:00:00 of this sunday
    double heading;     // [deg]
    double pitch;       // [deg]
    double roll;        // [deg]
    double gyro_x;      // optional, [deg/s]
    double gyro_y;      // optional, [deg/s]
    double gyro_z;      // optional, [deg/s]
    double acc_x;       // optional, [m/s^2]
    double acc_y;       // optional, [m/s^2]
    double acc_z;       // optional, [m/s^2]
    double latitude;    // [deg]
    double longitude;   // [deg]
    double altitude;    // [deg]
    double Ve;          // optional, [m/s]
    double Vn;          // optional, [m/s]
    double Vu;          // optional, [m/s]
    double baseline;    // optional, [m] baseline length
    int NSV1;           // optional, number of satellite of major antenna
    int NSV2;           // optional, number of satellite of secondary antenna
    int Status;         // optional, INS status
    int age;            // optional, latency of INS
    int Warnning;       // optional, INS Warnning
    std::string Cs;     // optional, verification
};

time_t TimeFromYMD(int year, int month, int day) {
  struct tm tm = {0};
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  return mktime(&tm);
}

#define SECS_PER_WEEK (60L*60*24*7)

int GPSweek() {
  time_t rawtime;
  time(&rawtime);
  double diff = difftime(rawtime, TimeFromYMD(1980, 1, 6));
  return (int) (diff / SECS_PER_WEEK);
}

int GPSsecond() {
  time_t rawtime;
  time(&rawtime);
  double diff = difftime(rawtime, TimeFromYMD(1980, 1, 6));
  return (diff - (int) (diff / SECS_PER_WEEK) * SECS_PER_WEEK);
}

std::string formatGPCHC(InsDataType ins) {
    char str[1024] = "";
    sprintf(str, "%s,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%.10lf,%.10lf,%.10lf,%lf,%lf,%lf,%d,%d,%d,%d,%d,%s",
            ins.header.c_str(), ins.gps_week, ins.gps_time, ins.heading, ins.pitch, ins.roll, ins.gyro_x, ins.gyro_y, ins.gyro_z,
            ins.acc_x, ins.acc_y, ins.acc_z, ins.latitude, ins.longitude, ins.altitude, ins.Ve, ins.Vn, ins.Vu,
            ins.baseline, ins.NSV1, ins.NSV2, ins.Status, ins.age, ins.Warnning, ins.Cs.c_str());
    return std::string(str);
}

int main(int argc, char **argv) {
    std::unique_ptr<UDPServer> udp_server(new UDPServer(12437));
    while (true) {
        InsDataType data;
        data.header   = "$GPCHC";
        data.gps_week = GPSweek();
        data.gps_time = GPSsecond();
        data.heading = 289.19;
        data.pitch = -0.42;
        data.roll = 0.21;
        data.gyro_x = -0.23;
        data.gyro_y = 0.07;
        data.gyro_z = -0.06;
        data.acc_x = 0.0009;
        data.acc_y = 0.0048;
        data.acc_z = -1.0037;
        data.latitude = 38.8594969;
        data.longitude = 121.5150073;
        data.altitude = 121.51;
        data.Ve = -0.023;
        data.Vn = 0.011;
        data.Vu = 0.0000;
        data.baseline = 1.500;
        data.NSV1 = 14;
        data.NSV2 = 6;
        data.Status = 4;
        data.age = 0;
        data.Warnning = 0;
        data.Cs = "*47";
        std::string message = formatGPCHC(data);
        udp_server->UDPSendtoBuf("127.0.0.1", 9888, (char *)(message.c_str()), message.size());
        usleep(10000);
    }
}