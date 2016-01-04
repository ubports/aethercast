/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */
#ifndef CORE_PROPERTY_H_
#define CORE_PROPERTY_H_

#include <core/signal.h>

#include <set>

namespace core
{
/**
 * @brief A very simple, templated class that allows for uniform declaration of get-able/set-able/observable members.
 *
 * A note on thread-safety: The property class itself does not give any thread-safety guarantees.
 * That is, consumers must not assume that concurrent get() and set() operations are synchronized by
 * this class.
 *
 * @tparam The type of the value contained within the property.
 */
template<typename T>
class Property
{
  public:
    /**
     * @brief ValueType refers to the type of the contained value.
     */
    typedef T ValueType;

    /**
     * @brief Getter refers to the function type for dispatching get operations to.
     */
    typedef std::function<ValueType()> Getter;
    
    /**
     * @brief Setter refers to the function type for dispatching set operations to.
     */
    typedef std::function<void(const ValueType&)> Setter;

    /**
     * @brief Property creates a new instance of property and initializes the contained value.
     * @param t The initial value, defaults to Property<T>::default_value().
     */
    inline explicit Property(const T& t = T{})
            : value{t},
              getter{},
              setter{}
    {
    }

    /**
     * @brief Copy c'tor, only copies the contained value, not the changed signal and its connections.
     * @param rhs
     */
    inline Property(const Property<T>& rhs) : value{rhs.value}
    {
    }

    inline virtual ~Property() = default;

    /**
     * @brief Assignment operator, only assigns to the contained value.
     * @param rhs The right-hand-side, raw value to assign to this property.
     */
    inline Property& operator=(const T& rhs)
    {
        set(rhs);
        return *this;
    }

    /**
     * @brief Assignment operator, only assigns to the contained value, not the changed signal and its connections.
     * @param rhs The right-hand-side property to assign from.
     */
    inline Property& operator=(const Property<T>& rhs)
    {
        set(rhs.value);
        return *this;
    }

    /**
     * @brief Explicit casting operator to the contained value type.
     * @return A non-mutable reference to the contained value.
     */
    inline operator const T&() const
    {
        return get();
    }

    /**
     * @brief Provides access to a pointer to the contained value.
     */
    inline const T* operator->() const
    {
        return &get();
    }

    /**
     * @brief operator == checks if the value of a property and a raw value are equal.
     * @param lhs Non-mutable reference to a property.
     * @param rhs Non-mutable reference to a raw value.
     * @return True iff the value contained in lhs equals rhs.
     */
    friend inline bool operator==(const Property<T>& lhs, const T& rhs)
    {
        return lhs.get() == rhs;
    }

    /**
     * @brief operator == checks if the value of two properties are equal.
     * @param lhs Non-mutable reference to a property.
     * @param rhs Non-mutable reference to a property.
     * @return True iff the value contained in lhs equals the value contained in rhs.
     */
    friend inline bool operator==(const Property<T>& lhs, const Property<T>& rhs)
    {
        return lhs.get() == rhs.get();
    }

    /**
     * @brief Set the contained value to the provided value. Notify observers of the change.
     * @param [in] new_value The new value to assign to this property.
     * @post get() == new_value;
     */
    inline virtual void set(const T& new_value)
    {
        if (value != new_value)
        {
            value = new_value;

            if (setter)
                setter(value);

            signal_changed(value);
        }
    }

    /**
     * @brief Access the value contained within this property.
     * @return A non-mutable reference to the property value.
     */
    inline virtual const T& get() const
    {
        if (getter)
            mutable_get() = getter();
        
        return value;
    }

    /**
     * @brief Access to the changed signal, allows observers to subscribe to change notifications.
     * @return A non-mutable reference to the changed signal.
     */
    inline const Signal<T>& changed() const
    {
        return signal_changed;
    }

    /**
     * @brief Provides in-place update facilities.
     *
     * The provided update_functor is applied to the contained value. If the update functor
     * returns true, indicating that the value has been changed, the changed signal is emitted.
     *
     * @param update_functor The update function to be applied to the contained value.
     * @return true iff application of the update functor has been successful.
     */
    inline virtual bool update(const std::function<bool(T& t)>& update_functor)
    {
        if (update_functor(mutable_get()))
        {
            signal_changed(value);
            return true;
        }

        return false;
    }

    /**
     * @brief install takes the provided functor and installs it for dispatching all set operations.
     * @param setter The functor to be invoked for set operations.
     */
    inline void install(const Setter& setter)
    {
        this->setter = setter;
    }

    /**
     * @brief install takes the provided functor and installs it for dispatching all get operations.
     * @param getter The functor to be invoked for get operations.
     */
    inline void install(const Getter& getter)
    {
        this->getter = getter;
    }

    friend inline const Property<T>& operator|(const Property<T>& lhs, Property<T>& rhs)
    {
        rhs.connections.emplace(
                    lhs.changed().connect(
                        std::bind(
                            &Property<T>::set,
                            std::ref(rhs),
                            std::placeholders::_1)));
        return lhs;
    }

  protected:
    inline virtual T& mutable_get() const
    {
        return value;
    }

  private:
    mutable T value;
    Getter getter;
    Setter setter;
    Signal<T> signal_changed;
    std::set<ScopedConnection> connections;
};
}

#endif // CORE_PROPERTY_H_
