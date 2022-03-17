#include <iostream>
#include <random>
#include <thread>
#include <string>
#include <sstream>
#include <deque>
#include <chrono>
#include <semaphore>
#include <mutex>
#include <fstream>

using namespace std;

deque<string> inputQueue;

mutex Reader;
mutex Writer;

void simulateInput(int rate)
{
    random_device rd{};
    mt19937 gen{ rd() };
    int count = 0;

    poisson_distribution<> pD(rate);
    normal_distribution<> nD(5.0, 3.0);
    uniform_int_distribution<int> uD(0, 10);

    while (true)
    {
        int numberEvents = pD(gen);
        if (numberEvents > 0)
        {
            for (int i = 0; i < numberEvents; ++i)
            {
                stringstream sampleStream;
                int deviceNumber = uD(gen);
                float sample = nD(gen);
                count++;

                {
                    std::lock_guard<std::mutex> writeLock(Writer);
                    sampleStream << count << " " << deviceNumber << " " << sample;
                    inputQueue.push_front(sampleStream.str());
                }
            }
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

void processData(int devNum)
{
    fstream dataStream;
    bool Flag = true;
    string filename = "device" + to_string(devNum);
    int count;
    int device;
    float value;
    dataStream.open(filename);

    while (Flag)
    {
        if (!inputQueue.empty())
        {
            std::lock(Reader, Writer);
            std::lock_guard<std::mutex> writeLock(Writer, std::adopt_lock);
            std::lock_guard<std::mutex> readLock(Reader, std::adopt_lock);
            string sample = inputQueue.back();
            stringstream sampleStringStream(sample);
            sampleStringStream >> count >> device >> value;
            if (device != 999)
            {
                if (devNum == device)
                {
                    inputQueue.pop_back();
                    dataStream << sample << endl;
                }
            }
            else
            {
                cout << "Device" << devNum << " has seen exit message" << endl;
                Flag = false;
            }
        }
    }
    dataStream.close(); //Closing the file
}

void Timer()
{
    bool keepgoing = true;
    int counter = 0;
    cout << "Five Minute Alarm: " << counter << " minutes have passed." << endl;
    while (keepgoing)
    {
        this_thread::sleep_for(chrono::seconds(5));
        ++counter;
        cout << "Five Minute Alarm: " << counter << " seconds have passed." << endl;
        if (counter == 5)
        {
            keepgoing = false;
        }
    }
    {
        std::lock_guard<std::mutex> writeLock(Writer);
        inputQueue.push_front("999 999 999");
    }
}


const int SAMPLE_ARRIVAL_RATE = 3;
int main(int argc, char* argv[])
{
    vector<thread> dataThreads;
    inputQueue.push_front("0 START");

    cout << "Starting writer thread" << endl;
    thread inputThread(simulateInput, SAMPLE_ARRIVAL_RATE);
    inputThread.detach();
    cout << "Starting simulation timer thread" << endl;
    thread timerThread(Timer);
    timerThread.detach();
    cout << "Pausing three seconds for station identification" << endl;
    this_thread::sleep_for(chrono::milliseconds(3000));
    // Now start printing stuff from the queue
    for (int i = 0; i < 10; ++i)
    {
        cout << "Starting reader number " << i << endl;
        dataThreads.push_back(thread(processData, i));
    }
    //wait for readers
    for (auto& thread : dataThreads)
    {
        thread.join();
    }
    while (!inputQueue.empty())
    {
        string sample = inputQueue.back();
        inputQueue.pop_back();
        cout << sample << endl;
    }
}
