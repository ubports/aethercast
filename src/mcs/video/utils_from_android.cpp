/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include "mcs/video/buffer.h"

namespace from_android {
bool GetNextNALUnit(
        const uint8_t **_data, size_t *_size,
        const uint8_t **nalStart, size_t *nalSize,
        bool startCodeFollows) {
    const uint8_t *data = *_data;
    size_t size = *_size;

    *nalStart = NULL;
    *nalSize = 0;

    if (size < 3) {
        return false;
    }

    size_t offset = 0;

    // A valid startcode consists of at least two 0x00 bytes followed by 0x01.
    for (; offset + 2 < size; ++offset) {
        if (data[offset + 2] == 0x01 && data[offset] == 0x00
                && data[offset + 1] == 0x00) {
            break;
        }
    }
    if (offset + 2 >= size) {
        *_data = &data[offset];
        *_size = 2;
        return false;
    }
    offset += 3;

    size_t startOffset = offset;

    for (;;) {
        while (offset < size && data[offset] != 0x01) {
            ++offset;
        }

        if (offset == size) {
            if (startCodeFollows) {
                offset = size + 2;
                break;
            }

            return false;
        }

        if (data[offset - 1] == 0x00 && data[offset - 2] == 0x00) {
            break;
        }

        ++offset;
    }

    size_t endOffset = offset - 2;
    while (endOffset > startOffset + 1 && data[endOffset - 1] == 0x00) {
        --endOffset;
    }

    *nalStart = &data[startOffset];
    *nalSize = endOffset - startOffset;

    if (offset + 2 < size) {
        *_data = &data[offset - 2];
        *_size = size - offset + 2;
    } else {
        *_data = NULL;
        *_size = 0;
    }

    return true;
}

bool IsIDR(const mcs::video::Buffer::Ptr &buffer) {
    const uint8_t *data = buffer->Data();
    size_t size = buffer->Length();

    bool foundIDR = false;

    const uint8_t *nalStart;
    size_t nalSize;
    while (GetNextNALUnit(&data, &size, &nalStart, &nalSize, true)) {
        unsigned nalType = nalStart[0] & 0x1f;
        if (nalType == 5) {
            foundIDR = true;
            break;
        }
    }

    return foundIDR;
}
} // namespace from_android
