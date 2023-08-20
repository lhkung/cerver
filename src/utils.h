#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>

namespace Cerver {

void LowerCase(std::string& str);
void Trim(std::string& str);
int Split(const std::string& str, const std::string& delim, std::vector<std::string>* out);
const char* GetTime();
bool IsNumber(const std::string& str);
bool EndsWith(const std::string& str, const std::string& suffix);
std::string RemoveExt(const std::string& str);

} // namespace Cerver
#endif