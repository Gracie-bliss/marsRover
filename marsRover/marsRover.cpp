#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstring>
#include <mutex>
#include<random>

const int NUM_SENSORS = 8;
const int READINGS_PER_HOUR = 60;
const int SHARED_MEMORY_SIZE = NUM_SENSORS * READINGS_PER_HOUR * sizeof(double);
const int REPORT_INTERVAL = 60 * 60 * 1000; 

struct Reading {
    double value;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
};

void generateReport(double* sharedMemory) {
    double* sensorMemory = sharedMemory;

    std::vector<double> highestTemps(5, -std::numeric_limits<double>::infinity());
    std::vector<double> lowestTemps(5, std::numeric_limits<double>::infinity());

    double largestDifference = -std::numeric_limits<double>::infinity();
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime;

    for (int i = 0; i < READINGS_PER_HOUR; i++) {
        std::vector<double> temperatures(NUM_SENSORS);

        for (int j = 0; j < NUM_SENSORS; j++) {
            Reading* sensorReading = reinterpret_cast<Reading*>(sensorMemory + j * READINGS_PER_HOUR + i);
            temperatures[j] = sensorReading->value;
        }

        std::sort(temperatures.begin(), temperatures.end());
        for (int j = 0; j < 5; j++) {\
            if (temperatures[j] > highestTemps[0]) {
                highestTemps[0] = temperatures[j];
                std::sort(highestTemps.begin(), highestTemps.end());
            }
        
            if (temperatures[j] < lowestTemps[4]) {
                lowestTemps[4] = temperatures[j];
                std::sort(lowestTemps.begin(), lowestTemps.end());
            }
        }


        double minTemp = *std::min_element(temperatures.begin(), temperatures.end());
        double maxTemp = *std::max_element(temperatures.begin(), temperatures.end());
        double difference = maxTemp - minTemp;
        if (difference > largestDifference) {
            largestDifference = difference;

            Reading* minReading = reinterpret_cast<Reading*>(sensorMemory + std::distance(temperatures.begin(), std::find(temperatures.begin(), temperatures.end(), minTemp)));
            Reading* maxReading = reinterpret_cast<Reading*>(sensorMemory + std::distance(temperatures.begin(), std::find(temperatures.begin(), temperatures.end(), maxTemp)));
            if (minReading->timestamp < maxReading->timestamp) {
                startTime = minReading->timestamp;
                endTime = maxReading->timestamp;
            }
            else {
                startTime = maxReading->timestamp;
                endTime = minReading->timestamp;
            }
        }
    }

    std::cout << "Report for the last hour:" << std::endl;
    std::cout << "Top 5 highest temperatures: ";
    for (double temperature : highestTemps) {
        std::cout << temperature << " ";
    }
    std::cout << std::endl;
    std::cout << "Top 5 lowest temperatures: ";
    for (double temperature : lowestTemps) {
        std::cout << temperature << " ";
    }
    std::cout << std::endl;
    std::cout << "Largest temperature difference: " << largestDifference << " (between "
        << std::chrono::system_clock::to_time_t(startTime) << " and "
        << std::chrono::system_clock::to_time_t(endTime) << ")" << std::endl;
}


void sensorFunction(int sensorId, std::mutex& mutex, double* sharedMemory, unsigned int seed) {

    double* sensorMemory = sharedMemory + sensorId * READINGS_PER_HOUR;

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(-100, 171);

    for (int i = 0; i < READINGS_PER_HOUR; i++) {
        int randomNum = dist(rng);

        Reading* sensorReading = reinterpret_cast<Reading*>(sensorMemory + i);
        sensorReading->value = randomNum;
        sensorReading->timestamp = std::chrono::system_clock::now();

        std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));

        std::lock_guard<std::mutex> lock(mutex);

    }

}


int main() {
    void* sharedMemory = std::malloc(SHARED_MEMORY_SIZE);
    std::memset(sharedMemory, 0, SHARED_MEMORY_SIZE);

    std::vector<std::thread> sensorThreads(NUM_SENSORS);
    std::mutex mutex;
    std::vector<unsigned int> seeds(NUM_SENSORS);
    std::random_device rd;
    for (int i = 0; i < NUM_SENSORS; i++) {
        seeds[i] = rd();
        sensorThreads[i] = std::thread(sensorFunction, i, std::ref(mutex), reinterpret_cast<double*>(sharedMemory), seeds[i]);
    }

    auto lastReportTime = std::chrono::system_clock::now();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(REPORT_INTERVAL));

        auto currentTime = std::chrono::system_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastReportTime);
        if (elapsedTime >= std::chrono::hours(1)) {
            generateReport(reinterpret_cast<double*>(sharedMemory));
            lastReportTime = currentTime;
        }

        if (currentTime - lastReportTime >= std::chrono::hours(1)) {
            break;
        }
    }

    for (int i = 0; i < NUM_SENSORS; i++) {
        sensorThreads[i].join();
    }

    std::free(sharedMemory);

    return 0;
}
