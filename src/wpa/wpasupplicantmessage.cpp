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

#include <inttypes.h>
#include <string.h>

#include <glib.h>

#include <mcs/utilities.h>

#include "wpasupplicantmessage.h"

WpaSupplicantMessage::WpaSupplicantMessage() :
    type_(kInvalid),
    iter_(0),
    sealed_(false) {
}

WpaSupplicantMessage::WpaSupplicantMessage(const WpaSupplicantMessage &other) :
    type_(other.type_),
    raw_(other.raw_),
    name_(other.name_),
    argv_(other.argv_),
    iter_(other.iter_),
    sealed_(other.sealed_) {
}

WpaSupplicantMessage::~WpaSupplicantMessage() {
}

WpaSupplicantMessage WpaSupplicantMessage::CreateRequest(const std::string &name) {
    WpaSupplicantMessage m;

    m.type_ = kRequest;
    m.name_ = name;

    return m;
}

WpaSupplicantMessage WpaSupplicantMessage::CreateRaw(const std::string &payload) {
    WpaSupplicantMessage m;

    if (payload.length() == 0) {
        m.type_ = kInvalid;
        return m;
    }

    std::string message;

    if (payload.at(0) == '<' && payload.length() > 3) {
        m.type_ = kEvent;
        message = payload.substr(3, payload.length() - 3);
    }
    else {
        m.type_ = kReply;
        message = payload;
    }

     m.Parse(message);

    return m;
}

std::string WpaSupplicantMessage::Name() const {
    return name_;
}

WpaSupplicantMessageType WpaSupplicantMessage::Type() const {
    return type_;
}

bool WpaSupplicantMessage::Sealed() const {
    return sealed_;
}

bool WpaSupplicantMessage::IsOk() const {
    return type_ == kReply && raw_ == "OK";
}

bool WpaSupplicantMessage::IsFail() const {
    return type_ == kReply && raw_ == "FAIL";
}

void WpaSupplicantMessage::Parse(const std::string &payload) {
    if (type_ == kInvalid)
        return;

    argv_.clear();

    std::vector<std::string> parts = mcs::utilities::StringSplit(payload, ' ');

    sealed_ = true;
    raw_ = payload;
    name_.assign(parts[0]);
    parts.erase(parts.begin());

    for (auto p : parts)
        AppendBasic('s', p.c_str());
}

bool WpaSupplicantMessage::Append(const char *types, ...) {
    va_list args;
    bool ret;

    if (sealed_)
        return false;

    va_start(args, types);
    ret = Appendv(types, &args);
    va_end(args);

    return ret;
}

bool WpaSupplicantMessage::Appendv(const char *types, va_list *args) {

    for (; *types; ++types) {
        if (!AppendvBasic(*types, args))
            return false;
    }

    return true;
}

bool WpaSupplicantMessage::AppendBasic(char type, ...) {
    va_list args;
    bool ret;

    va_start(args, type);
    ret = AppendvBasic(type, &args);
    va_end(args);

    return ret;
}

bool WpaSupplicantMessage::AppendvBasic(char type, va_list *args) {
    char *str = nullptr;
    const char *orig;
    const char *s, *t;
    char buf[128] = { };
    uint32_t u32;
    int32_t i32;

    switch (type) {
    case kString:
        orig = va_arg(*args, const char*);
        if (!orig)
            return false;
        break;
    case kInt32:
        i32 = va_arg(*args, int32_t);
        sprintf(buf, "%" PRId32, i32);
        orig = buf;
        break;
    case kUInt32:
        u32 = va_arg(*args, uint32_t);
        sprintf(buf, "%" PRIu32, u32);
        orig = buf;
        break;
    case kDict:
        s = va_arg(*args, const char*);
        if (!s)
            return false;

        t = va_arg(*args, const char*);
        if (!t)
            return false;

        str = g_strjoin("=", s, t, nullptr);
        if (!str)
            return false;
        break;
    default:
        break;
    }

    if (!str)
        str = g_strdup(orig);
    if (!str)
        return false;

    argv_.push_back(std::string(str));

    g_free(str);

    return true;
}

bool WpaSupplicantMessage::Read(const char *types, ...) {
    va_list args;
    void *arg;
    bool ret;

    va_start(args, types);

    for (; *types; ++types) {
        arg = va_arg(args, void*);
        ret = ReadBasic(*types, arg);
        if (!ret)
            break;
    }

    va_end(args);

    return true;
}

bool WpaSupplicantMessage::ReadBasic(char type, void *out) {
    if (argv_.size() == 0 || iter_ >= argv_.size() || !out)
        return false;

    const char *entry = argv_.at(iter_).c_str();

    switch (type) {
    case kString:
        *(const char**) out = entry;
        break;
    case kInt32:
        if (sscanf(entry, "%" SCNd32, (int32_t*) out) != 1)
            return false;
        break;
    case kUInt32:
        if (sscanf(entry, "%" SCNu32, (uint32_t*) out) != 1)
            return false;
        break;
    case kDict:
        entry = strchr(entry, '=');
        if (!entry)
            return false;

        *(const char**) out = entry + 1;
        break;
    default:
        return false;
    }

    ++iter_;
    return true;
}

bool WpaSupplicantMessage::ReadDictEntry(const std::string &name, char type, void *out) {
    const char *entry = nullptr;

    if (name.length() == 0 || !out)
        return false;

    for (int n = 0; n < argv_.size(); n++) {
        if (argv_[n].compare(0, name.length(), name) != 0)
            continue;

        entry = argv_[n].c_str();
        entry = strchr(entry, '=');
        if (!entry)
            continue;

        entry = entry + 1;

        switch (type) {
        case kString:
            *(const char**) out = entry;
            break;
        case kInt32:
            if (sscanf(entry, "%" SCNd32, (int32_t*) out) != 1)
                return false;
            break;
        case kUInt32:
            if (sscanf(entry, "%" SCNu32, (uint32_t*) out) != 1)
                return false;
            break;
        default:
            return false;
        }

        return true;
    }

    return false;
}

bool WpaSupplicantMessage::SkipBasic(char type) {
    if (iter_ >= argv_.size())
        return false;

    const char *entry = argv_.at(iter_).c_str();
    uint32_t u32;
    int32_t i32;

    switch (type) {
    case kString:
        break;
    case kInt32:
        if (sscanf(entry, "%" SCNd32, (int32_t*) &i32) != 1)
            return false;
        break;
    case kUInt32:
        if (sscanf(entry, "%" SCNu32, (uint32_t*) &u32) != 1)
            return false;
        break;
    case kDict:
        entry = strchr(entry, '=');
        if (!entry)
            return false;
        break;
    default:
        return false;
    }

    iter_++;
    return true;
}

bool WpaSupplicantMessage::Skip(const char *types) {
    for (; *types; ++types) {
        if (!SkipBasic(*types))
            return false;
    }
    return true;
}

void WpaSupplicantMessage::Rewind() {
    iter_ = 0;
}

void WpaSupplicantMessage::Seal() {
    raw_ = Dump();
    sealed_ = true;
}

std::string WpaSupplicantMessage::Raw() const {
    return raw_;
}

std::string WpaSupplicantMessage::Dump() const {
    std::string str = name_;
    int n = 0;
    for (auto p : argv_) {
        if (n < argv_.size())
            str += " ";
        str += p;
    }
    return str;
}
