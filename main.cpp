/**
 * @file main.cpp
 * @brief Windows serial application for sending pulse dir control data via serial
 * to Excelsior ECU.
 *
 * Simple app that sends serial data to test pulsedir on a windows computer.
 * Usage: excserial COM3 10 500
 * Sends 10 pulses alternating +/- with 500 Hz to COM3
 */

#include <chrono>
#include <format>
#include <iostream>
#include <thread>
#include <windows.h>

using namespace std::chrono_literals;

static std::atomic_bool gStopRequested{false};

BOOL WINAPI CtrlHandler(DWORD ctrlType) {
  switch (ctrlType) {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    gStopRequested = true;
    return TRUE;
  default:
    return FALSE;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cout << "Usage: winserial COM3 10 500 [Pulses with 10 pulses "
                 "alternating +/- at 500 Hz]"
              << std::endl;
    return EXIT_SUCCESS;
  }

  std::cout << "Starting excserial program..." << std::endl;

  // Validate the input
  int n;
  {
    std::stringstream ss(argv[2]);
    ss >> n;
    if (!ss) {
      std::cout << std::format("Can't convert arg {} to number!",
                               std::string_view(argv[2]))
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  int f;
  {
    std::stringstream ss(argv[3]);
    ss >> f;
    if (!ss) {
      std::cout << std::format("Can't convert arg {} to number!",
                               std::string_view(argv[2]))
                << std::endl;
      return EXIT_FAILURE;
    }
    if (f > 1000) {
      std::cout << "Frequency cant be bigger than 1000" << std::endl;
      return EXIT_FAILURE;
    }
  }
  std::chrono::milliseconds sleep_time_ms{1000 / f};

  // Bind to the com port
  std::string comport{argv[1]}; // Name of the com port
  const CHAR *pcCommPort = TEXT(argv[1]);
  HANDLE hCom = CreateFile(pcCommPort, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);

  if (hCom == INVALID_HANDLE_VALUE) {
    std::cout << std::format("CreateFile failed with error {}", GetLastError())
              << std::endl;
    return EXIT_FAILURE;
  }

  // Initialize DCB structure
  DCB dcb;
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);

  // Get current settings
  if (!GetCommState(hCom, &dcb)) {
    std::cout << std::format("GetCommState filed with error {}", GetLastError())
              << std::endl;
  }

  // Set settings
  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  std::cout << "Serial port successfully configured!" << std::endl;

  // Start sending
  std::cout << "Sending [+/-] " << n << " to " << comport << " with " <<
    f << "Hz (" << sleep_time_ms.count() << " ms..." << std::endl;
  while (!gStopRequested) {
    std::string msg = std::format("#{},{},{},{};", n, n, n, n);
    DWORD bytesWritten = 0;
    if (!WriteFile(hCom, msg.c_str(), static_cast<DWORD>(msg.size()),
                   &bytesWritten, nullptr)) {
      std::cout << "Failed to write to " << comport
                << "with error: " << std::to_string(GetLastError())
                << std::endl;
      CloseHandle(hCom);
      return EXIT_FAILURE;
    } else {
      n *= -1; // Flip n to alternate
    }
    std::this_thread::sleep_for(sleep_time_ms);
  }

  std::cout << "Got ctrl+c, exiting..." << std::endl;

  CloseHandle(hCom);

  return EXIT_SUCCESS;
}
