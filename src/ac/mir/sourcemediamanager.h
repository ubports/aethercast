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

#ifndef AC_MIR_SOURCEMEDIAMANAGERNEXT_H_
#define AC_MIR_SOURCEMEDIAMANAGERNEXT_H_

#include <memory>

#include "ac/glib_wrapper.h"

#include "ac/basesourcemediamanager.h"

#include "ac/common/executor.h"
#include "ac/common/threadedexecutor.h"
#include "ac/common/executorpool.h"

#include "ac/report/reportfactory.h"

#include "ac/network/stream.h"

#include "ac/video/baseencoder.h"

#include "ac/streaming/mediasender.h"

#include "ac/mir/screencast.h"
#include "ac/mir/streamrenderer.h"

namespace ac {
namespace mir {

class SourceMediaManager : public std::enable_shared_from_this<SourceMediaManager>,
                           public ac::BaseSourceMediaManager,
                           public ac::streaming::TransportSender::Delegate {
public:
    typedef std::shared_ptr<SourceMediaManager> Ptr;

    enum class State {
        Playing,
        Paused,
        Stopped
    };

    SourceMediaManager(const std::string &remote_address,
                       const ac::common::ExecutorFactory::Ptr &executor_factory,
                       const ac::video::BufferProducer::Ptr &producer,
                       const ac::video::BaseEncoder::Ptr &encoder,
                       const ac::network::Stream::Ptr &output_stream,
                       const ac::report::ReportFactory::Ptr &report_factory);

    ~SourceMediaManager();

    void Play() override;
    void Pause() override;
    void Teardown() override;
    bool IsPaused() const override;

    void SendIDRPicture() override;

    int GetLocalRtpPort() const override;

    void OnTransportNetworkError() override;

private:
    static gboolean OnStartPipeline(gpointer user_data);

    void CancelDelayTimeout();

protected:
    bool Configure() override;

private:
    State state_;
    std::string remote_address_;
    ac::video::BufferProducer::Ptr producer_;
    ac::video::BaseEncoder::Ptr encoder_;
    ac::network::Stream::Ptr output_stream_;
    ac::report::ReportFactory::Ptr report_factory_;
    ac::mir::StreamRenderer::Ptr renderer_;
    ac::streaming::MediaSender::Ptr sender_;
    ac::common::ExecutorPool pipeline_;
    guint delay_timeout_;
};

} // namespace mir
} // namespace ac

#endif
