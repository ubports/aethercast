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

#ifndef MCS_MIR_SOURCEMEDIAMANAGERNEXT_H_
#define MCS_MIR_SOURCEMEDIAMANAGERNEXT_H_

#include <memory>

#include "mcs/basesourcemediamanager.h"

#include "mcs/common/executor.h"

#include "mcs/video/baseencoder.h"

#include "mcs/streaming/mediasender.h"

#include "mcs/mir/screencast.h"
#include "mcs/mir/streamrenderer.h"

namespace mcs {
namespace mir {

class SourceMediaManager : public mcs::BaseSourceMediaManager {
public:
    typedef std::shared_ptr<SourceMediaManager> Ptr;

    enum class State {
        Playing,
        Paused,
        Stopped
    };

    static Ptr Create(const std::string &remote_address);

    ~SourceMediaManager();

    void Play() override;
    void Pause() override;
    void Teardown() override;
    bool IsPaused() const override;

    void SendIDRPicture() override;

    int GetLocalRtpPort() const override;

private:
    SourceMediaManager(const std::string &remote_address);

    void StartPipeline();
    void StopPipeline();

protected:
    bool Configure() override;

private:
    std::string remote_address_;
    State state_;
    mcs::video::BaseEncoder::Ptr encoder_;
    mcs::common::Executor::Ptr encoder_executor_;
    mcs::mir::Screencast::Ptr connector_;
    mcs::mir::StreamRenderer::Ptr renderer_;
    mcs::streaming::MediaSender::Ptr sender_;
};

} // namespace mir
} // namespace mcs

#endif
