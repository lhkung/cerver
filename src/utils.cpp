#include "utils.h"

using std::string;
using std::vector;

namespace Utils {

void LowerCase(string& str) {
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      str[i] = static_cast<char>(str[i] + 32);
    }
  }
}

void Trim(string& str) {
  int start = 0;
  int end = str.length() - 1;
  while (str[start] == ' ') {
    start++;
  }
  while (str[end] == ' ') {
    end--;
  }
  str = str.substr(start, end + 1);
}

int Split(const string& str, const string& delim, vector<string>* out) {
  size_t off = 0;
  while (true) {
    size_t index = str.find(delim, off);
    if (index == string::npos) {
      break;
    }
    if (index == off) {
      off++;
      continue;
    }
    out->push_back(str.substr(off, index - off));
    off = index + delim.length();
  }
  if (off < str.length()) {
    out->push_back(str.substr(off));
  }
  return out->size();
}

const char* GetTime() {
  time_t now = time(0);
  char* time_str = ctime(&now);
  int len = strlen(time_str);
  time_str[len - 1] = '\0';
  return strcat(time_str, ": ");
}

bool IsNumber(const string& str) {
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] < '0' || str[i] > '9') {
      return false;
    }
  }
  return true;
}

bool EndsWith(const string& str, const string& suffix) {
  if (suffix.length() == 0) {
    return true;
  }
  int i = str.length();
  int j = suffix.length();
  while (i >= 0 && j >= 0) {
    if (str[i] != suffix[j]) {
      break;
    }
    i--;
    j--;
  }
  if (j == -1) {
    return true;
  }
  return false;
}

string RemoveExt(const string& str) {
  string ret = "";
  for (int i = 0; i < str.length(); i++) {
    if (str[i] != '.') {
      ret += str[i];
    } else {
      break;
    }
  }
  return ret;
}

string GetExt(const std::string& str) {
  size_t dot = str.find(".");
  if (dot != string::npos) {
    return string(str, dot);
  }
  return "";
}

} // Utils