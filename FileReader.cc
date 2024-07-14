#include "FileReader.h"
#include <fstream>
#include <iostream>
#include <cstring>

FileReader::FileReader(char* filePath, int intervalMinutes, const std::string& prefix)
  : filePath(filePath), intervalMinutes(intervalMinutes), prefix(prefix), lastReadTime(std::chrono::steady_clock::now()) {
  if (filePath != nullptr) {
    readFile();
  } else {
    lastValue = prefix; // Initialize with prefix if no file is given
  }
}

FileReader::~FileReader() {
  // No dynamic memory to free
}

std::string FileReader::getValue() {
  auto now = std::chrono::steady_clock::now();
  auto elapsedMinutes = std::chrono::duration_cast<std::chrono::minutes>(now - lastReadTime).count();
  
  if (elapsedMinutes >= intervalMinutes && filePath != nullptr) {
    readFile();
    lastReadTime = now;
  }
  return lastValue;
}

void FileReader::readFile() {
  if (filePath == nullptr) {
    return; // Avoid trying to read if filePath is null
  }
  
  std::ifstream file(filePath);
  if (file.is_open()) {
    std::string line;
    std::string content = prefix; // Start with the prefix
    while (getline(file, line)) {
      content += line + "\n";
    }
    file.close();
    
    lastValue = content;
  } else {
    std::cerr << "Unable to open file: " << filePath << std::endl;
  }
}
