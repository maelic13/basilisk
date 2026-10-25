#include "version.h"

#include "basilisk_git_version.h"
#include "constants.h"

#include <iomanip>
#include <sstream>

namespace {

#ifndef BASILISK_GIT_DATE
#define BASILISK_GIT_DATE ""
#endif
#ifndef BASILISK_GIT_SHA
#define BASILISK_GIT_SHA ""
#endif
#ifndef BASILISK_DEV_ID
#define BASILISK_DEV_ID ""
#endif
#ifdef BASILISK_GIT_MODIFIED
constexpr bool git_modified = true;
#else
constexpr bool git_modified = false;
#endif

// The compiler's __DATE__ ("Sep 21 2008") as yyyymmdd.
std::string compile_date() {
    constexpr std::string_view months("Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec");
    std::string month, day, year;
    std::stringstream date(__DATE__);
    date >> month >> day >> year;
    std::ostringstream out;
    out << year << std::setw(2) << std::setfill('0') << (1 + months.find(month) / 4)
        << std::setw(2) << std::setfill('0') << day;
    return out.str();
}

} // namespace

std::string format_engine_version(std::string_view name, std::string_view version,
                                  std::string_view git_date, std::string_view git_sha,
                                  bool modified, std::string_view dev_id) {
    constexpr std::string_view dev_suffix = "-dev";
    const bool development = version.size() >= dev_suffix.size()
        && version.substr(version.size() - dev_suffix.size()) == dev_suffix;
    std::string out = std::string(name) + " ";
    if (!development)
        return out + std::string(version);
    out += "dev-";
    if (!dev_id.empty())
        return out + std::string(dev_id);
    out += git_date.empty() ? compile_date() : std::string(git_date);
    if (modified)
        out += "-m";
    out += "-";
    out += git_sha.empty() ? std::string("nogit") : std::string(git_sha);
    return out;
}

std::string engine_version_info() {
    return format_engine_version(engineName, engineVersion, BASILISK_GIT_DATE, BASILISK_GIT_SHA,
                                 git_modified, BASILISK_DEV_ID);
}
