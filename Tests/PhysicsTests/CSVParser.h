#pragma once
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include "PhysicExtension/Utils/PhysicsMath.h"

namespace lark::drone::test
{
    using namespace physics_math;

    class CSVParser
    {
        public:

            struct SimulationData
            {
                // Time
                float time;

                // Vehicle state
                Vector3f position;
                Vector3f velocity;
                Vector4f quaternion;
                Vector3f body_rates;
                Vector3f wind;
                Vector4f rotor_speeds;

                // Desired states (flat outputs)
                Vector3f position_des;
                Vector3f velocity_des;
                Vector3f acceleration_des;
                Vector3f jerk_des;
                Vector3f snap_des;
                float yaw_des;
                float yaw_dot_des;

                // IMU measurements
                Vector3f accel_measured;
                Vector3f accel_gt;
                Vector3f gyro;

                // Mocap measurements
                Vector3f mocap_position;
                Vector3f mocap_velocity;
                Vector4f mocap_quaternion;
                Vector3f mocap_body_rates;

                // Controller outputs
                Vector4f cmd_motor_speeds;
                float cmd_thrust;
                Vector4f cmd_quaternion;
                Vector3f cmd_moment;

                SimulationData() {
                    // Initialize all to zero
                    time = 0.0f;
                    position.setZero();
                    velocity.setZero();
                    quaternion = Vector4f(0, 0, 0, 1);
                    body_rates.setZero();
                    wind.setZero();
                    rotor_speeds.setZero();
                    position_des.setZero();
                    velocity_des.setZero();
                    acceleration_des.setZero();
                    jerk_des.setZero();
                    snap_des.setZero();
                    yaw_des = 0.0f;
                    yaw_dot_des = 0.0f;
                    accel_measured.setZero();
                    accel_gt.setZero();
                    gyro.setZero();
                    mocap_position.setZero();
                    mocap_velocity.setZero();
                    mocap_quaternion = Vector4f(0, 0, 0, 1);
                    mocap_body_rates.setZero();
                    cmd_motor_speeds.setZero();
                    cmd_thrust = 0.0f;
                    cmd_quaternion = Vector4f(0, 0, 0, 1);
                    cmd_moment.setZero();
                }
            };

        static std::vector<SimulationData> parseCSV(const std::string& filepath) {
            std::vector<SimulationData> data;
            std::ifstream file(filepath);

            if (!file.is_open()) {
                std::cerr << "Failed to open CSV file: " << filepath << std::endl;
                return data;
            }

            std::string line;

            // Read header line
            if (!std::getline(file, line)) {
                std::cerr << "CSV file is empty: " << filepath << std::endl;
                return data;
            }

            // Parse headers and create column mapping
            std::unordered_map<std::string, size_t> headerMap;
            std::stringstream headerStream(line);
            std::string header;
            size_t columnIndex = 0;

            while (std::getline(headerStream, header, ',')) {
                // Trim whitespace
                header.erase(0, header.find_first_not_of(" \t\r\n"));
                header.erase(header.find_last_not_of(" \t\r\n") + 1);
                headerMap[header] = columnIndex++;
            }

            // Validate that we have essential headers
            std::vector<std::string> requiredHeaders = {
                "time", "x", "y", "z", "xdot", "ydot", "zdot",
                "qx", "qy", "qz", "qw", "wx", "wy", "wz"
            };

            for (const auto& req : requiredHeaders) {
                if (headerMap.find(req) == headerMap.end()) {
                    std::cerr << "Missing required header: " << req << std::endl;
                    return data;
                }
            }

            // Read data rows
            while (std::getline(file, line)) {
                if (line.empty()) continue;

                SimulationData point;
                std::stringstream ss(line);
                std::string value;
                std::vector<float> rowData;

                // Parse all values in the row
                while (std::getline(ss, value, ',')) {
                    try {
                        rowData.push_back(std::stof(value));
                    } catch (const std::exception& e) {
                        rowData.push_back(0.0f);  // Default to 0 if parsing fails
                    }
                }

                // Map values to struct fields using header mapping
                point.time = getValueSafe(rowData, headerMap, "time");

                // Vehicle state
                point.position.x() = getValueSafe(rowData, headerMap, "x");
                point.position.y() = getValueSafe(rowData, headerMap, "y");
                point.position.z() = getValueSafe(rowData, headerMap, "z");

                point.velocity.x() = getValueSafe(rowData, headerMap, "xdot");
                point.velocity.y() = getValueSafe(rowData, headerMap, "ydot");
                point.velocity.z() = getValueSafe(rowData, headerMap, "zdot");

                point.quaternion.x() = getValueSafe(rowData, headerMap, "qx");
                point.quaternion.y() = getValueSafe(rowData, headerMap, "qy");
                point.quaternion.z() = getValueSafe(rowData, headerMap, "qz");
                point.quaternion.w() = getValueSafe(rowData, headerMap, "qw");

                point.body_rates.x() = getValueSafe(rowData, headerMap, "wx");
                point.body_rates.y() = getValueSafe(rowData, headerMap, "wy");
                point.body_rates.z() = getValueSafe(rowData, headerMap, "wz");

                point.wind.x() = getValueSafe(rowData, headerMap, "windx");
                point.wind.y() = getValueSafe(rowData, headerMap, "windy");
                point.wind.z() = getValueSafe(rowData, headerMap, "windz");

                point.rotor_speeds[0] = getValueSafe(rowData, headerMap, "r1");
                point.rotor_speeds[1] = getValueSafe(rowData, headerMap, "r2");
                point.rotor_speeds[2] = getValueSafe(rowData, headerMap, "r3");
                point.rotor_speeds[3] = getValueSafe(rowData, headerMap, "r4");

                // Desired states
                point.position_des.x() = getValueSafe(rowData, headerMap, "xdes");
                point.position_des.y() = getValueSafe(rowData, headerMap, "ydes");
                point.position_des.z() = getValueSafe(rowData, headerMap, "zdes");

                point.velocity_des.x() = getValueSafe(rowData, headerMap, "xdotdes");
                point.velocity_des.y() = getValueSafe(rowData, headerMap, "ydotdes");
                point.velocity_des.z() = getValueSafe(rowData, headerMap, "zdotdes");

                point.acceleration_des.x() = getValueSafe(rowData, headerMap, "xddotdes");
                point.acceleration_des.y() = getValueSafe(rowData, headerMap, "yddotdes");
                point.acceleration_des.z() = getValueSafe(rowData, headerMap, "zddotdes");

                point.jerk_des.x() = getValueSafe(rowData, headerMap, "xdddotdes");
                point.jerk_des.y() = getValueSafe(rowData, headerMap, "ydddotdes");
                point.jerk_des.z() = getValueSafe(rowData, headerMap, "zdddotdes");

                point.snap_des.x() = getValueSafe(rowData, headerMap, "xddddotdes");
                point.snap_des.y() = getValueSafe(rowData, headerMap, "yddddotdes");
                point.snap_des.z() = getValueSafe(rowData, headerMap, "zddddotdes");

                point.yaw_des = getValueSafe(rowData, headerMap, "yawdes");
                point.yaw_dot_des = getValueSafe(rowData, headerMap, "yawdotdes");

                // IMU measurements
                point.accel_measured.x() = getValueSafe(rowData, headerMap, "ax");
                point.accel_measured.y() = getValueSafe(rowData, headerMap, "ay");
                point.accel_measured.z() = getValueSafe(rowData, headerMap, "az");

                point.accel_gt.x() = getValueSafe(rowData, headerMap, "ax_gt");
                point.accel_gt.y() = getValueSafe(rowData, headerMap, "ay_gt");
                point.accel_gt.z() = getValueSafe(rowData, headerMap, "az_gt");

                point.gyro.x() = getValueSafe(rowData, headerMap, "gx");
                point.gyro.y() = getValueSafe(rowData, headerMap, "gy");
                point.gyro.z() = getValueSafe(rowData, headerMap, "gz");

                // Mocap measurements
                point.mocap_position.x() = getValueSafe(rowData, headerMap, "mocap_x");
                point.mocap_position.y() = getValueSafe(rowData, headerMap, "mocap_y");
                point.mocap_position.z() = getValueSafe(rowData, headerMap, "mocap_z");

                point.mocap_velocity.x() = getValueSafe(rowData, headerMap, "mocap_xdot");
                point.mocap_velocity.y() = getValueSafe(rowData, headerMap, "mocap_ydot");
                point.mocap_velocity.z() = getValueSafe(rowData, headerMap, "mocap_zdot");

                point.mocap_quaternion.x() = getValueSafe(rowData, headerMap, "mocap_qx");
                point.mocap_quaternion.y() = getValueSafe(rowData, headerMap, "mocap_qy");
                point.mocap_quaternion.z() = getValueSafe(rowData, headerMap, "mocap_qz");
                point.mocap_quaternion.w() = getValueSafe(rowData, headerMap, "mocap_qw");

                point.mocap_body_rates.x() = getValueSafe(rowData, headerMap, "mocap_wx");
                point.mocap_body_rates.y() = getValueSafe(rowData, headerMap, "mocap_wy");
                point.mocap_body_rates.z() = getValueSafe(rowData, headerMap, "mocap_wz");

                // Controller outputs
                point.cmd_motor_speeds[0] = getValueSafe(rowData, headerMap, "r1des");
                point.cmd_motor_speeds[1] = getValueSafe(rowData, headerMap, "r2des");
                point.cmd_motor_speeds[2] = getValueSafe(rowData, headerMap, "r3des");
                point.cmd_motor_speeds[3] = getValueSafe(rowData, headerMap, "r4des");

                point.cmd_thrust = getValueSafe(rowData, headerMap, "thrustdes");

                point.cmd_quaternion.x() = getValueSafe(rowData, headerMap, "qxdes");
                point.cmd_quaternion.y() = getValueSafe(rowData, headerMap, "qydes");
                point.cmd_quaternion.z() = getValueSafe(rowData, headerMap, "qzdes");
                point.cmd_quaternion.w() = getValueSafe(rowData, headerMap, "qwdes");

                point.cmd_moment.x() = getValueSafe(rowData, headerMap, "mxdes");
                point.cmd_moment.y() = getValueSafe(rowData, headerMap, "mydes");
                point.cmd_moment.z() = getValueSafe(rowData, headerMap, "mzdes");

                data.push_back(point);
            }

            file.close();

            std::cout << "Loaded " << data.size() << " data points from " << filepath << std::endl;
            return data;
        };

        static void printHeaders(const std::string& filepath) {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "Failed to open CSV file: " << filepath << std::endl;
                return;
            }

            std::string line;
            if (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string header;
                int index = 0;

                std::cout << "Headers in CSV file:" << std::endl;
                while (std::getline(ss, header, ',')) {
                    header.erase(0, header.find_first_not_of(" \t\r\n"));
                    header.erase(header.find_last_not_of(" \t\r\n") + 1);
                    std::cout << "  [" << index++ << "] " << header << std::endl;
                }
            }
            file.close();
        }

    private:
        // Safe value retrieval with bounds checking
        static float getValueSafe(const std::vector<float>& rowData,
                                  const std::unordered_map<std::string, size_t>& headerMap,
                                  const std::string& headerName) {
            auto it = headerMap.find(headerName);
            if (it != headerMap.end() && it->second < rowData.size()) {
                return rowData[it->second];
            }
            return 0.0f;  // Default value if header not found or index out of bounds
        }
    };
}