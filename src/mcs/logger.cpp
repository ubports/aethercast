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

#include "logger.h"

#define BOOST_LOG_DYN_LINK
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/manipulators.hpp>
#include <boost/log/utility/setup.hpp>

namespace {
namespace attrs {
BOOST_LOG_ATTRIBUTE_KEYWORD(Severity, "mcs::Severity", mcs::Logger::Severity)
BOOST_LOG_ATTRIBUTE_KEYWORD(File, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(Line, "Line", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(Timestamp, "Timestamp", boost::posix_time::ptime)
}

struct BoostLogLogger : public mcs::Logger {
    static boost::log::trivial::severity_level to_boost_log_severity(mcs::Logger::Severity severity) {
        switch (severity) {
        case mcs::Logger::Severity::kTrace: return boost::log::trivial::severity_level::trace;
        case mcs::Logger::Severity::kDebug: return boost::log::trivial::severity_level::debug;
        case mcs::Logger::Severity::kInfo: return boost::log::trivial::severity_level::info;
        case mcs::Logger::Severity::kWarning: return boost::log::trivial::severity_level::warning;
        case mcs::Logger::Severity::kError: return boost::log::trivial::severity_level::error;
        case mcs::Logger::Severity::kFatal: return boost::log::trivial::severity_level::fatal;
        default:
            return boost::log::trivial::severity_level::trace;
        }
    }

    BoostLogLogger() {
        boost::log::formatter formatter = boost::log::expressions::stream
            << "[" << attrs::Severity << " "
            << boost::log::expressions::format_date_time< boost::posix_time::ptime >("Timestamp", "%Y-%m-%d %H:%M:%S")
            << "] "
            << boost::log::expressions::smessage
            << boost::log::expressions::if_(boost::log::expressions::has_attr(attrs::File) && boost::log::expressions::has_attr(attrs::Line))
               [
                   boost::log::expressions::stream << " [" << attrs::File << ":" << attrs::Line << "]"
               ];

        boost::log::core::get()->remove_all_sinks();
        auto logger = boost::log::add_console_log(std::cout);
        logger->set_formatter(formatter);
        // logger->set_filter(attrs::Severity < mcs::Logger::Severity::kInfo);
    }

    void Log(Severity severity, const std::string& file, int line, const std::string& message) {
        if (auto rec = boost::log::trivial::logger::get().open_record()) {
            boost::log::record_ostream out{rec};
            out << boost::log::add_value(attrs::Severity, severity)
                << boost::log::add_value(attrs::Timestamp, boost::posix_time::microsec_clock::universal_time())
                << message;

            if (not file.empty() && line != -1) {
                out << boost::log::add_value(attrs::File, boost::filesystem::path(file).filename().string())
                    << boost::log::add_value(attrs::Line, line);
            }

            boost::log::trivial::logger::get().push_record(std::move(rec));
        }
    }
};

std::shared_ptr<mcs::Logger>& MutableInstance() {
    static std::shared_ptr<mcs::Logger> instance{new BoostLogLogger()};
    return instance;
}

void SetInstance(const std::shared_ptr<mcs::Logger>& logger) {
    MutableInstance() = logger;
}
}
namespace mcs {
void Logger::Trace(const std::string& file, int line, const std::string& message) {
    Log(Severity::kTrace, file, line, message);
}

void Logger::Debug(const std::string& file, int line, const std::string& message) {
    Log(Severity::kDebug, file, line, message);
}

void Logger::Info(const std::string& file, int line, const std::string& message) {
    Log(Severity::kInfo, file, line, message);
}

void Logger::Warning(const std::string& file, int line, const std::string& message) {
    Log(Severity::kWarning, file, line, message);
}

void Logger::Error(const std::string& file, int line, const std::string& message) {
    Log(Severity::kError, file, line, message);
}

void Logger::Fatal(const std::string& file, int line, const std::string& message) {
    Log(Severity::kFatal, file, line, message);
}

std::ostream& operator<<(std::ostream& strm, mcs::Logger::Severity severity) {
    switch (severity) {
    case mcs::Logger::Severity::kTrace: return strm << "TT";
    case mcs::Logger::Severity::kDebug: return strm << "DD";
    case mcs::Logger::Severity::kInfo: return strm << "II";
    case mcs::Logger::Severity::kWarning: return strm << "WW";
    case mcs::Logger::Severity::kError: return strm << "EE";
    case mcs::Logger::Severity::kFatal: return strm << "FF";
    default: return strm << static_cast<uint>(severity);
    }
}

Logger& Log() {
    return *MutableInstance();
}

void SetLogger(const std::shared_ptr<Logger>& logger) {
    SetInstance(logger);
}
}
