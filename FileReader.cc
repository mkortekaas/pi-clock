#include "FileReader.h"
#include <fstream>
#include <iostream>

FileReader::FileReader(const std::string& filePath, int intervalMinutes, const std::string& prefix)
  : filePath(filePath), intervalMinutes(intervalMinutes), prefix(prefix), lastReadTime(std::chrono::steady_clock::now()) {
  if (!filePath.empty()) {
    readFile();
  } else {
    lastValue = prefix; // Initialize with prefix if no file is given
  }
}

std::string FileReader::getValue() {
  auto now = std::chrono::steady_clock::now();
  auto elapsedMinutes = std::chrono::duration_cast<std::chrono::minutes>(now - lastReadTime).count();

  if (elapsedMinutes >= intervalMinutes && !filePath.empty()) {
    readFile();
    lastReadTime = now;
  }
  return lastValue;
}

void FileReader::readFile() {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Unable to open file: " << filePath << std::endl;
    return;
  }

  std::string line;
  std::string content = prefix; // Start with the prefix
  while (getline(file, line)) {
    content += line + "\n";
  }
  lastValue = content;
}
