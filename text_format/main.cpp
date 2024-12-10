#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>

void insertionSort(std::vector<int>& array, DWORD& iterationCount) {
    int n = array.size();
    for (int i = 1; i < n; i++) {
        for (int j = i; j > 0 && array[j - 1] > array[j]; j--) {
            std::swap(array[j - 1], array[j]);
            iterationCount++;
        }
    }
}

// Функция для записи чисел в текстовый файл
void WriteInitialNumbersToFile(const wchar_t* filename) {
    HANDLE hFile = CreateFile(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create file.\n";
        return;
    }

    const int arraySize = 10000000;
    std::vector<int> numbers(arraySize);

    for (int i = 0; i < arraySize; ++i) {
        numbers[i] = arraySize - i;
    }

    std::ostringstream oss;
    for (const auto& num : numbers) {
        oss << num << " ";
    }
    std::string dataStr = oss.str();
    DWORD bytesWritten;
    if (!WriteFile(hFile, dataStr.c_str(), dataStr.size(), &bytesWritten, NULL)) {
        std::cerr << "Failed to write numbers to file.\n";
    }
    else {
        std::cout << "Initial numbers written to file.\n";
    }

    CloseHandle(hFile);
}

DWORD WINAPI AsyncRead(LPVOID lpParam) {
    HANDLE hFile = *(HANDLE*)lpParam;
    OVERLAPPED ol = { 0 };
    ol.Offset = 0;
    ol.OffsetHigh = 0;

    const int BUF_SIZE = 4096;
    char buffer[BUF_SIZE] = { 0 };

    DWORD bytesRead = 0;
    BOOL result = ReadFile(hFile, buffer, BUF_SIZE, &bytesRead, &ol);

    if (!result && GetLastError() != ERROR_IO_PENDING) {
        std::cerr << "Failed to read file asynchronously.\n";
        return 1;
    }

    if (!GetOverlappedResult(hFile, &ol, &bytesRead, TRUE)) {
        std::cerr << "Failed to get overlapped result.\n";
        return 1;
    }

    std::cout << "Read " << bytesRead << " bytes.\n";

    std::istringstream iss(buffer);
    std::vector<int> data;
    int num;
    while (iss >> num) {
        data.push_back(num);
    }

    std::cout << "Data read from file (before sorting):\n";
    for (const auto& num : data) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    DWORD iterationCount = 0;
    insertionSort(data, iterationCount);

    std::cout << "Sorting completed. Iterations: " << iterationCount << "\n";

    std::cout << "Data after sorting:\n";
    for (const auto& num : data) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    return 0;
}

int main() {
    const wchar_t* filename = L"datafile.txt";

    WriteInitialNumbersToFile(filename);

    HANDLE hFile = CreateFile(
        filename,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    HANDLE hReadThread = CreateThread(NULL, 0, AsyncRead, &hFile, 0, NULL);
    WaitForSingleObject(hReadThread, INFINITE);

    CloseHandle(hFile);
    return 0;
}