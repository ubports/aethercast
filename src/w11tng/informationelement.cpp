/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <assert.h>
#include <iostream>
#include <string.h>
#include <netinet/in.h>

#include "informationelement.h"

namespace w11tng {

Subelement* new_subelement (SubelementId id)
{
    Subelement* element;
    switch (id) {
        case kDeviceInformation:
            element = reinterpret_cast<Subelement*>(new DeviceInformationSubelement);
            break;
        case kAssociatedBssid:
            element = reinterpret_cast<Subelement*>(new AssociatedBSSIDSubelement);
            break;
        case kCoupledSinkInformation:
            element = reinterpret_cast<Subelement*>(new CoupledSinkInformationSubelement);
            break;
        default:
            element = NULL;
            break;
    }

    if (element) {
        /* Fill in the common values */
        memset(element, 0, SubelementSize[id]);
        element->id = id;
        element->length = htons(SubelementSize[id] - 3);
    }

    return element;
}

void InformationElement::delete_subelement (Subelement *element)
{
    switch (element->id) {
        case kDeviceInformation:
            delete reinterpret_cast<DeviceInformationSubelement*>(element);
            break;
        case kAssociatedBssid:
            delete reinterpret_cast<AssociatedBSSIDSubelement*>(element);
            break;
        case kCoupledSinkInformation:
            delete reinterpret_cast<CoupledSinkInformationSubelement*>(element);
            break;
        default:
            assert(false);
    }
}

InformationElement::InformationElement(): length_(0) {}

InformationElement::InformationElement(const std::unique_ptr<InformationElementArray> &array)
{
    uint pos = 0;
    length_ = array->length;

    while (length_ >= pos + 2) {
        SubelementId id = static_cast<SubelementId>(array->bytes[pos]);
        size_t subelement_size = SubelementSize[id];

        Subelement *element = new_subelement(id);
        if (element) {
            memcpy (element, array->bytes + pos, subelement_size);
            subelements_[id] = element;
        }
        pos += subelement_size;
    }
}

InformationElement::~InformationElement()
{
    for (auto it = subelements_.begin(); it != subelements_.end(); it++){
        delete_subelement ((*it).second);
    }
    subelements_.clear();
}

void InformationElement::add_subelement(Subelement* subelement)
{
    SubelementId id = static_cast<SubelementId>(subelement->id);
    Subelement* old = subelements_[id];
    if (old){
        delete_subelement (old);
    } else {
        length_ += SubelementSize[id];
    }
    subelements_[id] = subelement;
}

DeviceType InformationElement::get_device_type() const
{
    auto it = subelements_.find (kDeviceInformation);
    if (it == subelements_.end()) {
        /* FIXME : exception ? */
        return kDualRole;
    }

    auto dev_info = reinterpret_cast<DeviceInformationSubelement*>((*it).second);
    return static_cast<DeviceType>(dev_info->field1.device_type);
}

int InformationElement::get_rtsp_port() const
{
    auto it = subelements_.find (kDeviceInformation);
    if (it == subelements_.end()) {
       /* FIXME : exception ? */
       return -1;
    }

    auto dev_info = reinterpret_cast<DeviceInformationSubelement*>((*it).second);
    return dev_info->session_management_control_port;
}

std::unique_ptr<InformationElementArray> InformationElement::serialize () const
{
    uint8_t pos = 0;
    std::unique_ptr<InformationElementArray> array
            (new InformationElementArray(length_));

    for (auto it = subelements_.begin(); it != subelements_.end(); it++) {
        Subelement* element = (*it).second;
        memcpy (array->bytes + pos, element, SubelementSize[element->id]);
        pos += SubelementSize[element->id];
    }

    return array;
}

std::string InformationElement::to_string() const
{
    std::string ret;

    auto array = serialize ();

    for (size_t i = 0; i < array->length; i++) {
        char hex[3];
        sprintf(hex,"%02X", array->bytes[i]);
        ret += hex;
    }

    return ret;
}

} // namespace w11tng

