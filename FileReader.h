#ifndef FILEREADER_H
#define FILEREADER_H

#include <string>
#include <chrono>

class FileReader {
public:
  // An empty filePath means "no file": getValue() then returns just the prefix
  FileReader(const std::string& filePath, int intervalMinutes, const std::string& prefix = "");
  std::string getValue();

private:
  std::string filePath;
  int intervalMinutes;
  std::string prefix;
  std::string lastValue;
  std::chrono::steady_clock::time_point lastReadTime;

  void readFile();
};

#endif // FILEREADER_H
