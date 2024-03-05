#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string.h>
#include <queue>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

// Mutex for synchronization
mutex trafficMutex;

// Number of producer and consumer threads
int numProducers = 2;
int numConsumers = 2;

// Constants for time tracking
int hourIndicator = 48;
int producerCounter = 0;
int consumerCounter = 0;
int totalRows = 0;

// Condition variables for signaling between producer and consumer
condition_variable producerCondition, consumerCondition;

// Data structures for holding traffic signal information
string indexStr, timestamp, trafficLightIdStr, numCarsStr;
vector<int> trafficIndices, trafficLightIds, numCars;
vector<string> trafficTimestamps;

// Struct to represent a traffic signal data row
struct TrafficData
{
    int index;
    std::string timestamp;
    int trafficLightId;
    int numCars;
};

// Array to hold totals of each of the 4 traffic lights
TrafficData trafficLightTotals[4] = { {0, "", 1, 0}, {0, "", 2, 0}, {0, "", 3, 0}, {0, "", 4, 0} };

// Queue to store traffic light data
queue<TrafficData> trafficDataQueue;
TrafficData currentTrafficData;

// Function to define sorting method for traffic signal data
bool compareTrafficData(TrafficData first, TrafficData second)
{
    return first.numCars > second.numCars;
}

// Producer function - reads data and pushes it into the queue
void* produceData(void* args)
{
    while (producerCounter < totalRows)
    {
        unique_lock<mutex> lock(trafficMutex);

        if (producerCounter < totalRows)
        {
            trafficDataQueue.push(TrafficData{trafficIndices[producerCounter], trafficTimestamps[producerCounter], trafficLightIds[producerCounter], numCars[producerCounter]});
            consumerCondition.notify_all();
            producerCounter++;
        }
        else
        {
            producerCondition.wait(lock, []{ return producerCounter < totalRows; });
        }

        lock.unlock();
        sleep(rand() % 3);
    }
}

// Consumer function - processes data from the queue
void* consumeData(void* args)
{
    while (consumerCounter < totalRows)
    {
        unique_lock<mutex> lock(trafficMutex);

        if (!trafficDataQueue.empty())
        {
            currentTrafficData = trafficDataQueue.front();

            // Add the number of cars into the respective traffic light id
            if (currentTrafficData.trafficLightId == 1)
            {
                trafficLightTotals[0].numCars += currentTrafficData.numCars;
            }
            if (currentTrafficData.trafficLightId == 2)
            {
                trafficLightTotals[1].numCars += currentTrafficData.numCars;
            }
            if (currentTrafficData.trafficLightId == 3)
            {
                trafficLightTotals[2].numCars += currentTrafficData.numCars;
            }
            if (currentTrafficData.trafficLightId == 4)
            {
                trafficLightTotals[3].numCars += currentTrafficData.numCars;
            }
            else {}

            trafficDataQueue.pop();
            producerCondition.notify_all();
            consumerCounter++;
        }
        else
        {
            consumerCondition.wait(lock, []{ return !trafficDataQueue.empty(); });
        }

        // Check if an hour has passed, sorting data and printing
        if (consumerCounter % hourIndicator == 0)
        {
            sort(trafficLightTotals, trafficLightTotals + 4, compareTrafficData);
            printf("Traffic lights sorted according to most activity | Time: %s \n", currentTrafficData.timestamp.c_str());
            cout << "Traffic Light" << setw(20) << "Number of Cars" << endl;

            for (int i = 0; i < 4; ++i)
            {
                cout << setw(3) << trafficLightTotals[i].trafficLightId << setw(20) << trafficLightTotals[i].numCars << endl;
            }
        }

        lock.unlock();
        sleep(rand() % 3);
    }
}

// Function to get traffic data from a file
void getTrafficData()
{
    ifstream infile;

    string filename;
    cout << "Greetings! From Mayank. Welcome to SIT 315 Traffic Control" << endl;
    cout << "Enter the name of the file: ";
    cin >> filename;

    infile.open(filename);

    if (infile.is_open())
    {
        std::string line;
        getline(infile, line);

        // Read data from the file and populate vectors
        while (!infile.eof())
        {
            getline(infile, indexStr, ',');
            trafficIndices.push_back(stoi(indexStr));
            getline(infile, timestamp, ',');
            trafficTimestamps.push_back(timestamp);
            getline(infile, trafficLightIdStr, ',');
            trafficLightIds.push_back(stoi(trafficLightIdStr));
            getline(infile, numCarsStr, '\n');
            numCars.push_back(stoi(numCarsStr));

            totalRows += 1;
        }
        infile.close();
    }
    else
    {
        printf("Unable to open the file.");
    }
}

int main()
{
    getTrafficData();

    pthread_t producerThreads[numProducers];
    pthread_t consumerThreads[numConsumers];

    // Create producer and consumer threads
    for(long i=0; i<numProducers; i++) pthread_create(&producerThreads[i], NULL, produceData, (void*) i);
    for(long i=0; i<numConsumers; i++) pthread_create(&consumerThreads[i], NULL, consumeData, (void*) i);

    // Join threads after completion
    for(long i=0; i<numProducers; i++) pthread_join(producerThreads[i], NULL);
    for(long i=0; i<numConsumers; i++) pthread_join(consumerThreads[i], NULL);
}
