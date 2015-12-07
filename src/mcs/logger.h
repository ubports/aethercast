/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "non_copyable.h"
#include "utils.h"

#include <string>

namespace mcs {
class Logger : public mcs::NonCopyable {
public:
    enum class Severity {
        kTrace,
        kDebug,
        kInfo,
        kWarning,
        kError,
        kFatal
    };

    class Record : public mcs::NonCopyable {
    public:

    };

    virtual void Log(Severity severity, const std::string& file, int line, const std::string &message) = 0;

    virtual void Trace(const std::string& file, int line, const std::string& message);
    virtual void Debug(const std::string& file, int line, const std::string& message);
    virtual void Info(const std::string& file, int line, const std::string& message);
    virtual void Warning(const std::string& file, int line, const std::string& message);
    virtual void Error(const std::string& file, int line, const std::string& message);
    virtual void Fatal(const std::string& file, int line, const std::string& message);

    template<typename... T>
    void Tracef(const std::string& file, int line, const std::string& pattern, T&&...args) { Trace(file, line, Utils::Sprintf(pattern, std::forward<T>(args)...)); }

    template<typename... T>
    void Debugf(const std::string& file, int line, const std::string& pattern, T&&...args) { Debug(file, line, Utils::Sprintf(pattern, std::forward<T>(args)...)); }

    template<typename... T>
    void Infof(const std::string& file, int line, const std::string& pattern, T&&...args) { Info(file, line, Utils::Sprintf(pattern, std::forward<T>(args)...)); }

    template<typename... T>
    void Warningf(const std::string& file, int line, const std::string& pattern, T&&...args) { Warning(file, line, Utils::Sprintf(pattern, std::forward<T>(args)...)); }

    template<typename... T>
    void Errorf(const std::string& file, int line, const std::string& pattern, T&&...args) { Error(file, line, Utils::Sprintf(pattern, std::forward<T>(args)...)); }

    template<typename... T>
    void Fatalf(const std::string& file, int line, const std::string& pattern, T&&...args) { Fatal(file, line, Utils::Sprintf(pattern, std::forward<T>(args)...)); }

protected:
    Logger() = default;
};

// operator<< inserts severity into out.
std::ostream& operator<<(std::ostream& out, Logger::Severity severity);

// Log returns the mcs-wide configured logger instance.
// Save to call before/after main.
Logger& Log();
// SetLog installs the given logger as mcs-wide default logger.
void SetLogger(const std::shared_ptr<Logger>& logger);

#define TRACE(...) Log().Tracef(__FILE__, __LINE__, __VA_ARGS__)
#define DEBUG(...) Log().Debugf(__FILE__, __LINE__, __VA_ARGS__)
#define INFO(...) Log().Infof(__FILE__, __LINE__, __VA_ARGS__)
#define WARNING(...) Log().Warningf(__FILE__, __LINE__, __VA_ARGS__)
#define ERROR(...) Log().Errorf(__FILE__, __LINE__, __VA_ARGS__)
#define FATAL(...) Log().Fatalf(__FILE__, __LINE__, __VA_ARGS__)
}

#define MCS_TRACE(...) mcs::Log().Tracef(__FILE__, __LINE__, __VA_ARGS__)
#define MCS_DEBUG(...) mcs::Log().Debugf(__FILE__, __LINE__, __VA_ARGS__)
#define MCS_INFO(...) mcs::Log().Infof(__FILE__, __LINE__, __VA_ARGS__)
#define MCS_WARNING(...) mcs::Log().Warningf(__FILE__, __LINE__, __VA_ARGS__)
#define MCS_ERROR(...) mcs::Log().Errorf(__FILE__, __LINE__, __VA_ARGS__)
#define MCS_FATAL(...) mcs::Log().Fatalf(__FILE__, __LINE__, __VA_ARGS__)

#endif
