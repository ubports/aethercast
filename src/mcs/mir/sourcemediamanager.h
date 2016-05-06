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

#include "mcs/glib_wrapper.h"

#include "mcs/basesourcemediamanager.h"

#include "mcs/common/executor.h"
#include "mcs/common/threadedexecutor.h"
#include "mcs/common/executorpool.h"

#include "mcs/report/reportfactory.h"

#include "mcs/network/stream.h"

#include "mcs/video/baseencoder.h"

#include "mcs/streaming/mediasender.h"

#include "mcs/mir/screencast.h"
#include "mcs/mir/streamrenderer.h"

namespace mcs {
namespace mir {

class SourceMediaManager : public std::enable_shared_from_this<SourceMediaManager>,
                           public mcs::BaseSourceMediaManager,
                           public mcs::streaming::TransportSender::Delegate {
public:
    typedef std::shared_ptr<SourceMediaManager> Ptr;

    enum class State {
        Playing,
        Paused,
        Stopped
    };

    SourceMediaManager(const std::string &remote_address,
                       const mcs::common::ExecutorFactory::Ptr &executor_factory,
                       const mcs::video::BufferProducer::Ptr &producer,
                       const mcs::video::BaseEncoder::Ptr &encoder,
                       const mcs::network::Stream::Ptr &output_stream,
                       const mcs::report::ReportFactory::Ptr &report_factory);

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
    mcs::video::BufferProducer::Ptr producer_;
    mcs::video::BaseEncoder::Ptr encoder_;
    mcs::network::Stream::Ptr output_stream_;
    mcs::report::ReportFactory::Ptr report_factory_;
    mcs::mir::StreamRenderer::Ptr renderer_;
    mcs::streaming::MediaSender::Ptr sender_;
    mcs::common::ExecutorPool pipeline_;
    guint delay_timeout_;
};

} // namespace mir
} // namespace mcs

#endif
