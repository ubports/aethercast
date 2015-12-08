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

#include <mcs/utils.h>

#include "wpasupplicantmessage.h"

WpaSupplicantMessage WpaSupplicantMessage::CreateRequest(const std::string &name) {
    WpaSupplicantMessage m;

    m.type_ = Type::kRequest;
    m.name_ = name;

    return m;
}

WpaSupplicantMessage WpaSupplicantMessage::Parse(const std::string &payload) {
    WpaSupplicantMessage m;

    if (payload.length() == 0) {
        m.type_ = Type::kInvalid;
        return m;
    }

    std::string message;

    if (payload.at(0) == '<' && payload.length() > 3) {
        m.type_ = Type::kEvent;
        message = payload.substr(3, payload.length() - 3);
    }
    else {
        m.type_ = Type::kReply;
        message = payload;
    }

    std::vector<std::string> parts = mcs::Utils::StringSplit(message, ' ');

    m.raw_ = payload;
    m.name_ = (parts[0]);
    m.argv_.insert(m.argv_.end(), parts.begin() + 1, parts.end());

    m.sealed_ = true;
    m.iter_ = m.argv_.begin();

    return m;
}

const std::string& WpaSupplicantMessage::get_name() const {
    return name_;
}

WpaSupplicantMessage::Type WpaSupplicantMessage::get_type() const {
    return type_;
}

bool WpaSupplicantMessage::is_sealed() const {
    return sealed_;
}

bool WpaSupplicantMessage::IsOk() const {
    return type_ == Type::kReply && raw_ == "OK";
}

bool WpaSupplicantMessage::IsFail() const {
    return type_ == Type::kReply && raw_ == "FAIL";
}

void WpaSupplicantMessage::Rewind() {
    iter_ = argv_.begin();
}

void WpaSupplicantMessage::Seal() {
    raw_ = Dump();
    sealed_ = true;
}

const std::string& WpaSupplicantMessage::get_raw() const {
    return raw_;
}

std::string WpaSupplicantMessage::Dump() const {
    std::stringstream ss; ss << name_;
    for (const auto& arg : argv_) {
        ss << " " << arg;
    }
    return ss.str();
}

void WpaSupplicantMessage::ThrowIfAtEnd() const {
    if (iter_ == argv_.end()) {
        throw std::out_of_range{"WpaSupplicantMessage: No more argument to extract"};
    }
}

void WpaSupplicantMessage::ThrowIfSealed() const {
    if (sealed_) {
        throw std::logic_error{"WpaSupplicantMessage: Cannot append data to sealed message."};
    }
}
