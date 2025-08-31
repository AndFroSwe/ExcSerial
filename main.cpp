/**
 * @file main.cpp
 * @brief Windows serial application for sending alternating messages to serial.
 *
 * Simple app that sends serial data on a windows computer.
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

inline std::string error_string(DWORD error_code) {
  char *buffer = nullptr;
  const DWORD len = ::FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<char *>(&buffer), 0, nullptr);

  if (len == 0 || buffer == nullptr)
    return "Unknown error (" + std::to_string(error_code) + ")";

  std::string result{buffer};
  ::LocalFree(buffer);
  // Remove trailing CRLF
  while (!result.empty() && (result.back() == '\r' || result.back() == '\n'))
    result.pop_back();
  return result;
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cout << "Usage: excserial COM3 10 500 [Pulses with 10 pulses "
                 "alternating +/- at 500 Hz]"
              << std::endl;
    return EXIT_SUCCESS;
  }

  std::cout << "Starting excserial program..." << std::endl;

  // Validate the input
  int n; // Number to sent each iteration
  {
    std::stringstream ss(argv[2]);
    ss >> n;
    if (!ss) {
      std::cerr << std::format("Can't convert arg {} to number!",
                               std::string_view(argv[2]))
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  int f; // Frequency to send
  {
    std::stringstream ss(argv[3]);
    ss >> f;
    if (!ss) {
      std::cerr << std::format("Can't convert arg {} to number!",
                               std::string_view(argv[2]))
                << std::endl;
      return EXIT_FAILURE;
    }
    if (f > 1000) {
      std::cerr << "Frequency cant be bigger than 1000" << std::endl;
      return EXIT_FAILURE;
    }
  }
  std::chrono::milliseconds sleep_time_ms{1000 / f};

  // Bind to the com port
  std::string_view comport{argv[1]}; // Name of the com port
  const CHAR *pcCommPort = TEXT(argv[1]);
  HANDLE hCom = CreateFile(pcCommPort, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);

  if (hCom == INVALID_HANDLE_VALUE) {
    std::cerr << std::format("CreateFile failed with error: {}",
                             error_string(GetLastError()))
              << std::endl;
    return EXIT_FAILURE;
  }

  // Validation done
  // Handle ctrl+c
  if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
    std::cerr << "Failed to set control handler: "
              << error_string(GetLastError()) << std::endl;
    return EXIT_FAILURE;
  }

  // Initialize DCB structure for com port
  DCB dcb;
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);

  // Get current settings
  if (!GetCommState(hCom, &dcb)) {
    std::cerr << std::format("GetCommState filed with error {}",
                             error_string(GetLastError()))
              << std::endl;
  }

  // Set settings
  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  std::cout << "Serial port successfully configured!" << std::endl;

  // Without this timeout is infinite
  COMMTIMEOUTS timeouts = {0};
  timeouts.ReadIntervalTimeout = 50;
  timeouts.ReadTotalTimeoutConstant = 10;
  timeouts.ReadTotalTimeoutMultiplier = 10;
  timeouts.WriteTotalTimeoutConstant = 50;
  timeouts.WriteTotalTimeoutMultiplier = 10;
  if (!SetCommTimeouts(hCom, &timeouts)) {
    CloseHandle(hCom);
    std::cerr << "Could not set timeouts with error: "
              << error_string(GetLastError()) << std::endl;
    return EXIT_FAILURE;
  }

  // Setup send loop
  constexpr auto status_print_time = 2s;
  unsigned int messages_sent = 0;
  auto last_send_time = std::chrono::steady_clock::now();
  auto last_print_time = last_send_time;

  std::cout << "Sending [+/-] " << n << " to " << comport << " with " << f
            << "Hz (" << sleep_time_ms.count() << " ms)..." << std::endl;

  // Start the loop
  while (!gStopRequested) {
    // Busy wait loop since windows can't do sub 16 ms sleep with chrono
    while (std::chrono::steady_clock::now() - last_send_time < sleep_time_ms) {
      Sleep(0); // Yield CPU
    }
    const auto now = std::chrono::steady_clock::now();
    last_send_time = now;

    // Try to send
    std::string msg = std::format("#{},{},{},{};", n, n, n, n);
    DWORD bytesWritten = 0;
    if (!WriteFile(hCom, msg.c_str(), static_cast<DWORD>(msg.size()),
                   &bytesWritten, nullptr)) {
      std::cerr << "Failed to write to " << comport
                << " with error: " << error_string(GetLastError()) << std::endl;
      CloseHandle(hCom);
      return EXIT_FAILURE;
    } else {
      n *= -1; // Flip n to alternate
      messages_sent++;
    }

    // Status print
    if (now - last_print_time > status_print_time) {
      std::cout << '\r' << std::string(120, ' ');
      std::cout << "\rMessages sent: " << messages_sent << std::flush;
      last_print_time = now;
    }
  }

  std::cout << std::endl << "Got ctrl+c, exiting..." << std::endl;

  CloseHandle(hCom);

  return EXIT_SUCCESS;
}
