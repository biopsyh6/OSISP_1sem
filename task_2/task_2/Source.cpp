#include <windows.h>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <chrono>
#include <vector>

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, 1);

int total_readers = 5;
int total_writers = 2;
const std::string FILENAME1 = "data1.txt";
const std::string FILENAME2 = "data2.txt";
double readers_wait_time = 0;
double writers_wait_time = 0;

HANDLE mutex[2];
HANDLE writeSemaphore[2];
HANDLE readSemaphore[2];
int readCount[2] = { 0, 0 };
int writeWaitCount = 0;

std::string getRandomFile() {
    int randomValue = dis(gen);
    return (randomValue % 2) ? FILENAME1 : FILENAME2;
}

DWORD WINAPI Writer(LPVOID param) {
    int id = *(int*)param;

    std::string filename = getRandomFile();
    int indx = -1;
    if (filename == FILENAME1) {
        indx = 0;
    }
    else {
        indx = 1;
    }

    std::cout << "Writer " << id << " is waiting to write in " << filename << " ...\n";

    auto start = std::chrono::high_resolution_clock::now();

    WaitForSingleObject(mutex[indx], INFINITE);                // Эта функция блокирует текущий поток до тех пор, пока семафор writeSemaphore не станет доступным.
    WaitForSingleObject(writeSemaphore[indx], INFINITE);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "Waiting time taken: " << duration.count() << " ms" << std::endl;

    writers_wait_time += duration.count();

    std::ofstream file(filename, std::ios::app);
    if (file.is_open()) {
        file << "Writer " << id << " wrote data at " << GetTickCount64() << "\n";
        std::cout << "Writer " << id << " is writing to " << filename << "...\n";
        Sleep(300);
        file.close();
    }
    else {
        std::cout << "Failed to open " << filename << " for writing.\n";
    }

    Sleep(400);

    ReleaseMutex(mutex[indx]);
    ReleaseSemaphore(writeSemaphore[indx], 1, NULL);
    std::cout << "Writer " << id << " done its work!\n";

    return 0;
}

DWORD WINAPI Reader(LPVOID param) {
    int id = *(int*)param;

    std::string filename = getRandomFile();
    int indx = -1;
    if (filename == FILENAME1) {
        indx = 0;
    }
    else {
        indx = 1;
    }

    Sleep(50);
    std::cout << "Reader " << id << " is waiting to read from " << filename << " ...\n";

    auto start = std::chrono::high_resolution_clock::now();

    WaitForSingleObject(mutex[indx], INFINITE);
    WaitForSingleObject(readSemaphore[indx], INFINITE);
    readCount[indx]++;

    if (readCount[indx] == 1) {
        WaitForSingleObject(writeSemaphore[indx], INFINITE);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "Waiting time taken: " << duration.count() << " ms" << std::endl;

    readers_wait_time += duration.count();

    ReleaseMutex(mutex[indx]);

    std::ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        std::string text;
        Sleep(400);
        while (std::getline(file, line)) {
            text += line + '\n';
        }
        std::cout << "Reader " << id << " read: " << text.length() << " chars" << "\n";

        file.close();
    }
    else {
        std::cout << "Failed to open " << filename << " for reading.\n";
    }

    Sleep(700);

    WaitForSingleObject(mutex[indx], INFINITE);
    readCount[indx]--;
    if (readCount[indx] == 0) {
        ReleaseSemaphore(writeSemaphore[indx], 1, NULL);
    }
    //ReleaseMutex(mutex[indx]);
    std::cout << "Reader " << id << " done its work!\n";

    return 0;
}

int main() {
    srand(static_cast<unsigned int>(time(0)));
    mutex[0] = CreateMutexW(NULL, FALSE, NULL);
    mutex[1] = CreateMutexW(NULL, FALSE, NULL);
    writeSemaphore[0] = CreateSemaphoreW(NULL, 1, 1, NULL);
    writeSemaphore[1] = CreateSemaphoreW(NULL, 1, 1, NULL);
    readSemaphore[0] = CreateSemaphoreW(NULL, 5, 5, NULL);
    readSemaphore[1] = CreateSemaphoreW(NULL, 5, 5, NULL);

    int q = 0;

    //std::cin >> q;
    std::vector<int> query;

    if (q == 0) {
        query = { 1, 2, 1, 2, 2, 1, 2 };
        //query = { 2, 2, 2, 1};
    }
    else {
        int size;
        std::cout << "Enter the query to run threads, where 1 is Writer, 2 is Reader (max 20): ";
        std::cin >> size;

        std::cout << "Enter " << size << " integers:" << std::endl;
        for (int i = 0; i < size; ++i) {
            int value;
            std::cin >> value;
            query.push_back(value);
        }
    }
    //подсчет количества потоков каждого типа
    int count1 = 0;
    int count2 = 0;

    for (int num : query) {
        if (num == 1) {
            count1++;
        }
        else if (num == 2) {
            count2++;
        }
    }

    int readerIndex = 0;
    int writerIndex = 0;

    HANDLE threads[20];
    int ids[20];

    for (int i = 0; i < query.size(); i++) {
        if (query[i] == 1) {
            ids[i] = writerIndex;
            std::cout << "Writer " << writerIndex << " is starting... \n";
            writerIndex++;
            threads[i] = CreateThread(NULL, 0, Writer, &ids[i], 0, NULL);
            continue;
        }
        if (query[i] == 2) {
            ids[i] = readerIndex;
            std::cout << "Reader " << readerIndex << " is starting... \n";
            readerIndex++;
            threads[i] = CreateThread(NULL, 0, Reader, &ids[i], 0, NULL);
            continue;
        }
        for (int l = 0; l < 100000; l++) {
            l;
        }
    }

    WaitForMultipleObjects(query.size(), threads, TRUE, INFINITE);

    for (int i = 0; i < query.size(); i++) {
        CloseHandle(threads[i]);
    }

    CloseHandle(mutex[0]);
    CloseHandle(mutex[1]);
    CloseHandle(writeSemaphore[0]);
    CloseHandle(writeSemaphore[1]);
    CloseHandle(readSemaphore[0]);
    CloseHandle(readSemaphore[1]);

    std::cout << "\nOveral readers wait time: " << readers_wait_time << "ms\n";
    std::cout << "Average reader wait time: " << readers_wait_time / count2 << "ms\n\n";
    std::cout << "Overal writers wait time: " << writers_wait_time << "ms\n";
    std::cout << "Average writer wait time: " << writers_wait_time / count1 << "ms\n";

    return 0;
}