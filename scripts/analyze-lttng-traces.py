#!/usr/bin/python3

import sys
from collections import Counter
import babeltrace
import statistics

def analyze_trace(path):
    col = babeltrace.TraceCollection()
    if col.add_trace(path, 'ctf') is None:
        raise RuntimeError('Cannot add trace')

    buffers = {}

    for event in col.events:
        if event.name.startswith("aethercast_"):
            buffer_timestamp = event["timestamp"]

            current_buffer = {}

            if buffer_timestamp in buffers.keys():
                current_buffer = buffers[buffer_timestamp]
            else:
                current_buffer['timestamp'] = buffer_timestamp

            current_buffer[event.name] = event.timestamp

            buffers[buffer_timestamp] = current_buffer

    rendering_times = []
    encoding_times = []
    packetizing_times = []
    sending_times = []

    for buffer_timestamp in buffers:
        buffer = buffers[buffer_timestamp]

        start = buffer["aethercast_encoder:received_input_buffer"]
        usec_per_sec = 1000000

        renderer_end = buffer["aethercast_renderer:finished_frame"]
        rendering_times.append(((renderer_end - start) / usec_per_sec))

        encoder_end = buffer["aethercast_encoder:finished_frame"]
        encoding_times.append(((encoder_end - start) / usec_per_sec))

        packetizer_end = buffer["aethercast_packetizer:packetized_frame"]
        packetizing_times.append(((packetizer_end - start) / usec_per_sec))

        sender_end = buffer["aethercast_sender:sent_packet"]
        sending_times.append(((sender_end - start) / usec_per_sec))

    def dump_statistics(name, data):
        print("%s time max: %f ms min: %f ms mean: %f ms stdev: %f ms" %
              (name, max(data), min(data), statistics.mean(data), statistics.stdev(data)))

    dump_statistics("Rendering", rendering_times)
    dump_statistics("Encoding", encoding_times)
    dump_statistics("Packetizing", packetizing_times)
    dump_statistics("Sending", sending_times)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: {} TRACEPATH'.format(sys.argv[0]))
        sys.exit(1)

    analyze_trace(sys.argv[1])
