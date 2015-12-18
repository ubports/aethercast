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

#include <glib.h>

#include <mcs/utils.h>

#include "message.h"

namespace w11t {
Message Message::CreateRequest(const std::string &name) {
    Message m;

    m.type_ = Type::kRequest;
    m.name_ = name;

    return m;
}

Message Message::Parse(const std::string &payload) {
    Message m;

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

    for (auto it = parts.begin() + 1; it != parts.end(); ++it) {
        auto pos = it->find("=");
        if (pos != std::string::npos) {
            m.optional_args_[it->substr(0, pos)] = it->substr(pos+1);
        } else {
            m.positional_args_.push_back(*it);
        }
    }

    m.sealed_ = true;
    m.iter_ = m.positional_args_.begin();

    return m;
}

const std::string& Message::Name() const {
    return name_;
}

Message::Type Message::ItsType() const {
    return type_;
}

bool Message::Sealed() const {
    return sealed_;
}

bool Message::IsOk() const {
    return type_ == Type::kReply && raw_ == "OK";
}

bool Message::IsFail() const {
    return type_ == Type::kReply && raw_ == "FAIL";
}

void Message::Rewind() {
    iter_ = positional_args_.begin();
}

void Message::Seal() {
    raw_ = Dump();
    sealed_ = true;
}

const std::string& Message::Raw() const {
    return raw_;
}

std::string Message::Dump() const {
    std::stringstream ss; ss << name_;
    for (const auto& arg : positional_args_) {
        ss << " " << arg;
    }
    for (const auto& pair : optional_args_) {
        ss << " " << pair.first << "=" << pair.second;
    }
    return ss.str();
}

void Message::ThrowIfAtEnd() const {
    if (iter_ == positional_args_.end()) {
        throw std::out_of_range{"WpaSupplicantMessage: No more argument to extract"};
    }
}

void Message::ThrowIfSealed() const {
    if (sealed_) {
        throw std::logic_error{"WpaSupplicantMessage: Cannot append data to sealed message."};
    }
}
}
