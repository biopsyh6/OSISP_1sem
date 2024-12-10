#include <windows.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <ctime>

#define MIN_BUFFER_SIZE 2048

DWORD iterationCountAsync = 0;
DWORD iterationCount = 0;

void insertionSort(std::vector<int>& array, DWORD& iterationCount) {
    int n = array.size();
    for (int i = 1; i < n; i++) {
        for (int j = i; j > 0 && array[j - 1] > array[j]; j--) {
            std::swap(array[j - 1], array[j]);
            iterationCount++;
        }
    }
}

void writeToFile(const std::vector<int>& numbers, const std::wstring& filename) {
    std::wofstream outFile(filename);
    if (outFile.is_open()) {
        for (const int& number : numbers) {
            outFile << number << L" ";
        }
        outFile.close();
    }
    else {
        std::wcerr << L"Error opening file for writing: " << filename << std::endl;
    }
}

void generateNumbersFile(const std::wstring& filename, int count) {
    std::wofstream outFile(filename);
    if (outFile.is_open()) {
        //std::srand(std::time(0));
        //for (int i = 0; i < count; i++) {
        //    outFile << std::rand() % 1000 << L" ";
        //}
        for (int i = 0; i < count; i++) {
            outFile << count - i << L" ";
        }
        outFile.close();
    }
    else {
        std::wcerr << L"Error opening file for writing: " << filename << std::endl;
    }
}

void CALLBACK ReadCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped) {
    if (dwErrorCode == 0) {
        lpOverlapped->Offset += dwNumberOfBytesTransferred; //offset for position
        char* buffer = (char*)lpOverlapped->hEvent;
        buffer[dwNumberOfBytesTransferred] = '\0';

        std::vector<int> numbers;
        std::istringstream iss(buffer);
        int number;
        while (iss >> number) {
            numbers.push_back(number);
        }

        insertionSort(numbers, iterationCountAsync);
        writeToFile(numbers, L"sorted_data_async.txt");
    }
    else {
        if (dwErrorCode == 38) {
            return;
        }
        std::cerr << "Error reading file: " << dwErrorCode << std::endl;
    }
}

void actionAsync(int bufferSize) {
    HANDLE hFile = CreateFile(
        L"data.txt",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file: " << GetLastError() << std::endl;
        return;
    }

    char* buffer = new char[bufferSize];
    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = (HANDLE)buffer; //Use this buffer for async operation

    while (true) {
        DWORD prev_offset = overlapped.Offset; //read from this position
        if (!ReadFileEx(hFile, buffer, bufferSize - 1, &overlapped, ReadCompletionRoutine)) { //call callback after async read
            std::cerr << "Error initiating read: " << GetLastError() << std::endl;
            CloseHandle(hFile);
            return;
        }

        SleepEx(INFINITE, TRUE);
        if (prev_offset == overlapped.Offset) { //if position is not replaced
            break;
        }
    }

    CloseHandle(hFile);
    delete[] buffer;
}

void action() {
    HANDLE hFile = CreateFile(
        L"data.txt",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Could not open file (Error: " << GetLastError() << ")." << std::endl;
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        std::cerr << "Could not get file size (Error: " << GetLastError() << ")." << std::endl;
        CloseHandle(hFile);
        return;
    }

    char* buffer = new char[fileSize + 1];
    if (!buffer) {
        std::cerr << "Memory allocation failed." << std::endl;
        CloseHandle(hFile);
        return;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
        std::cerr << "Could not read file (Error: " << GetLastError() << ")." << std::endl;
        delete[] buffer;
        CloseHandle(hFile);
        return;
    }

    buffer[bytesRead] = '\0';

    std::vector<int> numbers;
    std::istringstream iss(buffer);
    int number;
    while (iss >> number) {
        numbers.push_back(number);
    }

    insertionSort(numbers, iterationCount);
    writeToFile(numbers, L"sorted_data.txt");

    delete[] buffer;
    CloseHandle(hFile);
}

int main() {
    generateNumbersFile(L"data.txt", 5000);

    int i = 0;
    bool exit = false;
    while (!exit) {
        int buff_exp = pow(2, i); //up buffer size
        i++;
        auto startAsync = std::chrono::high_resolution_clock::now();
        if (MIN_BUFFER_SIZE * buff_exp >= 1e9) {
            exit = true;
        }
        actionAsync(MIN_BUFFER_SIZE * buff_exp);
        auto endAsync = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> durationAsync = endAsync - startAsync;
        std::cout << "Async: Time: " << durationAsync.count() << " " << "Buffer size: " << MIN_BUFFER_SIZE * buff_exp << " " << "Iterations: " << iterationCountAsync << " " << "Perfomance: " << (iterationCountAsync / durationAsync.count()) * 0.00000001 << "\n";
        iterationCountAsync = 0;
    }

    auto start = std::chrono::high_resolution_clock::now();
    action();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    std::cout << "Default: Time: " << duration.count() << " " << "Iterations: " << iterationCount << " " << "Perfomance: " << (iterationCount / duration.count()) * 0.00000001 << "\n";
    return 0;
}
