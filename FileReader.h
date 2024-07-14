#ifndef FILEREADER_H
#define FILEREADER_H

#include <string>
#include <chrono>

class FileReader {
public:
  FileReader(char* filePath, int intervalMinutes, const std::string& prefix = "");
  ~FileReader();
  std::string getValue();

private:
  char* filePath; // Keep filePath as a char* to directly handle nullptr
  int intervalMinutes;
  std::string prefix;
  std::string lastValue;
  std::chrono::steady_clock::time_point lastReadTime;
  
  void readFile();
};

#endif // FILEREADER_H
