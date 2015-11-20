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

#ifndef WPASUPPLICANTMESSAGE_H_
#define WPASUPPLICANTMESSAGE_H_

#include <string>
#include <vector>

#include <stdarg.h>

enum WpaSupplicantMessageType {
    kInvalid = 0,
    kEvent,
    kRequest,
    kReply
};

enum WpaSupplicantMessageValueType {
    kString = 's',
    kInt32 = 'i',
    kUInt32 = 'u',
    kDict = 'e'
};

class WpaSupplicantMessage {
public:
    static WpaSupplicantMessage CreateRequest(const std::string &Name);
    static WpaSupplicantMessage CreateRaw(const std::string &payload);

    WpaSupplicantMessage(const WpaSupplicantMessage &other);
    ~WpaSupplicantMessage();

    bool Append(const char *types, ...);
    bool Read(const char *types, ...);
    bool ReadDictEntry(const std::string &name, char type, void *out);
    bool Skip(const char *types);
    void Rewind();
    void Seal();
    std::string Dump() const;

    bool IsOk() const;
    bool IsFail() const;

    std::string Name() const;
    WpaSupplicantMessageType Type() const;
    bool Sealed() const;
    std::string Raw() const;

private:
    WpaSupplicantMessage();

    void Parse(const std::string &payload);

    bool Appendv(const char *types, va_list *args);
    bool AppendvBasic(char Type, va_list *args);
    bool AppendBasic(char Type, ...);
    bool ReadBasic(char Type, void *out);
    bool SkipBasic(char Type);

private:
    std::string raw_;
    std::string name_;
    WpaSupplicantMessageType type_;
    std::vector<std::string> argv_;
    unsigned int iter_;
    bool sealed_;
};

#endif
