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

#include <memory>

#include <wds/media_manager.h>

#include "ac/non_copyable.h"

namespace ac {
class BaseSourceMediaManager : public wds::SourceMediaManager
{
public:
    class Delegate : public ac::NonCopyable {
    public:
        virtual void OnSourceNetworkError() = 0;
    };

    explicit BaseSourceMediaManager();

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);
    void ResetDelegate();

    void SetSinkRtpPorts(int port1, int port2) override;
    std::pair<int,int> GetSinkRtpPorts() const override;
    virtual int GetLocalRtpPort() const override;
    wds::SessionType GetSessionType() const override;

    bool InitOptimalVideoFormat(const wds::NativeVideoFormat& sink_native_format,
                                const std::vector<wds::H264VideoCodec>& sink_supported_codecs) override;
    wds::H264VideoFormat GetOptimalVideoFormat() const override;
    bool InitOptimalAudioFormat(const std::vector<wds::AudioCodec>& sink_supported_codecs) override;
    wds::AudioCodec GetOptimalAudioFormat() const override;
    virtual void SendIDRPicture() override;
    std::string GetSessionId() const override;

protected:
    virtual bool Configure() = 0;
    virtual std::vector<wds::H264VideoCodec> GetH264VideoCodecs();

    std::weak_ptr<Delegate> delegate_;

protected:
    int sink_port1_;
    int sink_port2_;
    wds::H264VideoFormat format_;
    wds::AudioCodec audio_codec_;
    unsigned int session_id_;
};
} // namespace ac
#endif
