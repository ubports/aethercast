/*
 * Copyright (C) 2016 Canonical, Ltd.
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

#ifndef MCS_TOOLS_SIMPLESOURCE_H_
#define MCS_TOOLS_SIMPLESOURCE_H_

#include <string>
#include <memory>

#include "mcs/basesourcemediamanager.h"

namespace mcs {
namespace tools {

class SimpleSource {
public:
    typedef std::shared_ptr<SimpleSource> Ptr;

    static Ptr Create(const std::string &remote_address, int port);

    ~SimpleSource();

    void Start();
    void Stop();

private:
    SimpleSource(const std::string &remote_address, int port);

private:
    std::shared_ptr<BaseSourceMediaManager> media_manager_;
    std::vector<wds::H264VideoCodec> sink_codecs_;
    wds::NativeVideoFormat sink_native_format_;
};

} // namespace tools
} // namespace mcs

#endif
