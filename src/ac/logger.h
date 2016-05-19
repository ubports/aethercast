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

#include <boost/optional.hpp>

#include <string>

namespace ac {
// A Logger enables persisting of messages describing & explaining the
// state of the system.
class Logger : public ac::NonCopyable {
public:
    // Severity enumerates all known severity levels
    // applicable to log messages.
    enum class Severity {
        kTrace,
        kDebug,
        kInfo,
        kWarning,
        kError,
        kFatal
    };

    // A Location describes the origin of a log message.
    struct Location {
        std::string file; // The name of the file that contains the log message.
        std::string function; // The function that contains the log message.
        std::uint32_t line; // The line in file that resulted in the log message.
    };

    virtual void Init(const ac::Logger::Severity &severity = ac::Logger::Severity::kWarning) = 0;

    virtual void Log(Severity severity, const std::string &message, const boost::optional<Location>& location) = 0;

    virtual void Trace(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Debug(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Info(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Warning(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Error(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Fatal(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});


    template<typename... T>
    void Tracef(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Trace(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Debugf(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Debug(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Infof(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Info(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Warningf(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Warning(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Errorf(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Error(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Fatalf(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Fatal(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

protected:
    Logger() = default;
};

// operator<< inserts severity into out.
std::ostream& operator<<(std::ostream& out, Logger::Severity severity);

// operator<< inserts location into out.
std::ostream& operator<<(std::ostream& out, const Logger::Location &location);

// Log returns the mcs-wide configured logger instance.
// Save to call before/after main.
Logger& Log();
// SetLog installs the given logger as mcs-wide default logger.
void SetLogger(const std::shared_ptr<Logger>& logger);

#define TRACE(...) Log().Tracef(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define DEBUG(...) Log().Debugf(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define INFO(...) Log().Infof(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define WARNING(...) Log().Warningf(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define ERROR(...) Log().Errorf(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define FATAL(...) Log().Fatalf(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
}

#define AC_TRACE(...) ac::Log().Tracef(ac::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define AC_DEBUG(...) ac::Log().Debugf(ac::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define AC_INFO(...) ac::Log().Infof(ac::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define AC_WARNING(...) ac::Log().Warningf(ac::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define AC_ERROR(...) ac::Log().Errorf(ac::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define AC_FATAL(...) ac::Log().Fatalf(ac::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)

#endif
