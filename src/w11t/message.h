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
#include <unordered_map>
#include <vector>

namespace w11t {
template<typename V>
class Named {
public:
    explicit Named(const std::string& key, const V& value = V{}) :
        key_{key},
        value_{value} {
    }

    const std::string& Key() const {
        return key_;
    }

    V& Value() {
        return value_;
    }

    const V& Value() const {
        return value_;
    }

    operator V() const {
        return value_;
    }

private:
    std::string key_;
    V value_;
};

template<typename T>
struct Skip {
};

template<typename T>
inline T& skip() {
    static T t;
    return t;
}

class Message {
public:
    static Message CreateRequest(const std::string &Name);
    static Message Parse(const std::string &payload);

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

    const Message& Read() const {
        return *this;
    }

    template<typename T, typename... Tail>
    const Message& Read(Named<T>& named, Tail&&... tail) const {
        auto it = optional_args_.find(named.Key());
        if (it != optional_args_.end()) {
            std::stringstream ss{it->second}; ss >> named.Value();
        }
        return Read(std::forward<Tail>(tail)...);
    }

    template<typename Head, typename... Tail>
    const Message& Read(Head& head, Tail&&... tail) const {
        ThrowIfAtEnd();
        std::stringstream ss{*iter_}; ss >> head;
        ++iter_;
        return Read(std::forward<Tail>(tail)...);
    }

    Message& Write() {
        return *this;
    }

    template<typename Head, typename... Tail>
    Message& Write(const Head& head, const Tail&... tail) {
        ThrowIfSealed();
        std::stringstream ss; ss << head;
        positional_args_.push_back(ss.str());
        return Write(std::forward<Tail>(tail)...);
    }

    template<typename T, typename... Tail>
    Message& Write(const Named<T>& named, const Tail&... tail) {
        ThrowIfSealed();
        std::stringstream ss; ss << named.Value();
        optional_args_[named.Key()] = ss.str();
        return Write(std::forward<Tail>(tail)...);
    }

    Message Write() const {
        return *this;
    }

    template<typename Head, typename... Tail>
    Message Write(const Head& head, const Tail&... tail) const {
        Message that{*this};
        return that.Write(head, std::forward<Tail>(tail)...);
    }

    const std::string& Name() const;
    Type ItsType() const;
    bool Sealed() const;
    const std::string& Raw() const;

private:
    Message() = default;

    void ThrowIfAtEnd() const;
    void ThrowIfSealed() const;

private:
    std::string raw_;
    std::string name_;
    Type type_ = Type::kInvalid;
    std::vector<std::string> positional_args_;
    std::unordered_map<std::string, std::string> optional_args_;
    mutable std::vector<std::string>::iterator iter_ = positional_args_.begin();
    bool sealed_ = false;
};

template<typename V>
inline bool operator!=(const Named<V>& lhs, const V& rhs) {
    return lhs.Value() != rhs;
}

template<typename V>
inline bool operator!=(const V& lhs, const Named<V>& rhs) {
    return rhs.Value() != lhs;
}

template<typename V>
inline bool operator==(const Named<V>& lhs, const V& rhs) {
    return lhs.Value() == rhs;
}

template<typename V>
inline bool operator==(const V& lhs, const Named<V>& rhs) {
    return rhs.Value() == lhs;
}

template<typename V>
inline std::ostream& operator<<(std::ostream& out, const Named<V>& entry) {
    return out << entry.Key() << "=" << entry.Value();
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
inline Message& operator<<(Message &msg, const T& field) {
    return msg.Write(field);
}

template<typename T>
inline Message operator<<(const Message &msg, const T& field) {
    return msg.Write(field);
}
}
#endif
