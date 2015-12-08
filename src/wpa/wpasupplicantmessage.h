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

#include <sstream>
#include <string>
#include <vector>

#include <mcs/mac_address.h>
#include <mcs/utils.h>

#include <stdarg.h>

class WpaSupplicantMessage {
public:
    static WpaSupplicantMessage CreateRequest(const std::string &get_name);
    static WpaSupplicantMessage Parse(const std::string &payload);

    enum class Type {
        kInvalid = 0,
        kEvent,
        kRequest,
        kReply
    };

    void Rewind();
    void Seal();
    std::string Dump() const;

    bool IsOk() const;
    bool IsFail() const;    

    const WpaSupplicantMessage& Read() const {
        return *this;
    }

    template<typename Head, typename... Tail>
    const WpaSupplicantMessage& Read(Head& head, Tail&&... tail) const {
        ThrowIfAtEnd();
        std::stringstream ss{*iter_}; ss >> head;
        ++iter_;
        return Read(std::forward<Tail>(tail)...);
    }

    WpaSupplicantMessage& Write() {
        return *this;
    }

    template<typename Head, typename... Tail>
    WpaSupplicantMessage& Write(const Head& head, Tail&&... tail) {
        ThrowIfSealed();
        std::stringstream ss; ss << head;
        argv_.push_back(ss.str());
        return Write(std::forward<Tail>(tail)...);
    }

    WpaSupplicantMessage Write() const {
        return *this;
    }

    template<typename Head, typename... Tail>
    WpaSupplicantMessage Write(const Head& head, Tail&&... tail) const {
        WpaSupplicantMessage that{*this};
        return that.Write(head, std::forward<Tail>(tail)...);
    }

    const std::string& get_name() const;
    Type get_type() const;
    bool is_sealed() const;
    const std::string& get_raw() const;

private:
    WpaSupplicantMessage() = default;

    void ThrowIfAtEnd() const;
    void ThrowIfSealed() const;

private:
    std::string raw_;
    std::string name_;
    Type type_ = Type::kInvalid;
    std::vector<std::string> argv_;
    mutable std::vector<std::string>::iterator iter_ = argv_.begin();
    bool sealed_ = false;
};

template<typename K, typename V>
struct Named {
    operator V() const {
        return value;
    }

    K key;
    V value;
};

template<typename K, typename V>
inline bool operator!=(const Named<K, V>& lhs, const V& rhs) {
    return lhs.value != rhs;
}

template<typename K, typename V>
inline bool operator!=(const V& lhs, const Named<K, V>& rhs) {
    return rhs != lhs;
}

template<typename K, typename V>
inline bool operator==(const Named<K, V>& lhs, const V& rhs) {
    return lhs.value == rhs;
}

template<typename K, typename V>
inline bool operator==(const V& lhs, const Named<K, V>& rhs) {
    return rhs == lhs;
}

template<typename T>
struct Skip {
};

template<typename T>
inline T& skip() {
    static T t;
    return t;
}

template<typename K, typename V>
inline std::ostream& operator<<(std::ostream& out, const Named<K, V>& entry) {
    return out << entry.key << "=" << entry.value;
}

template<typename K, typename V>
inline std::istream& operator>>(std::istream& in, Named<K, V>& entry) {
    std::string s; in >> s;
    auto pos = s.find("=");

    if (pos != std::string::npos) {
        std::stringstream sk{s.substr(0, pos)}; sk >> entry.key;
        std::stringstream sv{s.substr(pos+1)}; sv >> entry.value;
    }

    return in;
}

template<typename T>
inline std::ostream& operator<<(std::ostream& out, Skip<T>) {
    return out;
}

template<typename T>
inline std::istream& operator>>(std::istream& in, Skip<T>) {
    T t;
    return in >> t;
}

template<typename T>
inline WpaSupplicantMessage& operator<<(WpaSupplicantMessage &msg, const T& field) {
    return msg.Write(field);
}

template<typename T>
inline WpaSupplicantMessage operator<<(const WpaSupplicantMessage &msg, const T& field) {
    return msg.Write(field);
}

#endif
