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

    encoding_times=[]

    for buffer_timestamp in buffers:
        buffer = buffers[buffer_timestamp]
        start = buffer["aethercast_encoder:received_input_buffer"]
        end = buffer["aethercast_encoder:finished_frame"]
        encoding_times.append(((end - start) / 1000000))

    print("Encoding time max: %f ms min: %f ms mean: %f ms stdev: %f ms" %
          (max(encoding_times), min(encoding_times),
           statistics.mean(encoding_times), statistics.stdev(encoding_times)))

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: {} TRACEPATH'.format(sys.argv[0]))
        sys.exit(1)

    analyze_trace(sys.argv[1])
