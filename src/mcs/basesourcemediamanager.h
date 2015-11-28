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

#ifndef BASEMEDIAMANAGER_H_
#define BASEMEDIAMANAGER_H_

#include <wds/media_manager.h>

namespace mcs {
class BaseSourceMediaManager : public wds::SourceMediaManager
{
public:
    void SetSinkRtpPorts(int port1, int port2) override;
    std::pair<int,int> GetSinkRtpPorts() const override;
    int GetLocalRtpPort() const override;
    wds::SessionType GetSessionType() const override;

    bool InitOptimalVideoFormat(const wds::NativeVideoFormat& sink_native_format,
                                const std::vector<wds::H264VideoCodec>& sink_supported_codecs) override;
    wds::H264VideoFormat GetOptimalVideoFormat() const override;
    bool InitOptimalAudioFormat(const std::vector<wds::AudioCodec>& sink_supported_codecs) override;
    wds::AudioCodec GetOptimalAudioFormat() const override;
    void SendIDRPicture() override;

protected:
    virtual void Configure() = 0;

protected:
    int sink_port1_;
    int sink_port2_;
    wds::H264VideoFormat format_;
};
} // namespace mcs
#endif
