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

#ifndef GSTMEDIAMANAGER_H_
#define GSTMEDIAMANAGER_H_

#include <glib.h>
#include <gst/gst.h>

#include "basesourcemediamanager.h"

namespace mcs {
class GstSourceMediaManager : public BaseSourceMediaManager
{
public:
    explicit GstSourceMediaManager();
    ~GstSourceMediaManager();

    void Play() override;
    void Pause() override;
    void Teardown() override;
    bool IsPaused() const override;

protected:
    void Configure() override;

    virtual GstElement* ConstructPipeline(const wds::H264VideoFormat &format) = 0;

private:
    static gboolean OnGstBusEvent(GstBus *bus, GstMessage *message, gpointer data);

private:
    GstElement *pipeline_;
    std::string remote_address_;
    guint bus_watch_id_;
};
} // namespace mcs
#endif
