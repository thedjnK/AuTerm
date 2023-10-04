/******************************************************************************
** Copyright (C) 2017 Intel Corporation.
**
** Project: AuTerm
**
** Module:  crc16.h
**
** Notes:   Taken from Zephyr source
**
** License: Licensed under the Apache License, Version 2.0 (the "License");
**          you may not use this file except in compliance with the License.
**          You may obtain a copy of the License at
**
**             http://www.apache.org/licenses/LICENSE-2.0
**
**          Unless required by applicable law or agreed to in writing, software
**          distributed under the License is distributed on an "AS IS" BASIS,
**          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
**          implied. See the License for the specific language governing
**          permissions and limitations under the License.
**
*******************************************************************************/
#include "crc16.h"

uint16_t crc16(const QByteArray *src, size_t i, size_t len, uint16_t polynomial,
               uint16_t initial_value, bool pad)
{
    uint16_t crc = initial_value;
    size_t padding = pad ? sizeof(crc) : 0;
    size_t b;

    /* src length + padding (if required) */
    while (i < (len + padding))
    {
        for (b = 0; b < 8; b++) {
            uint16_t divide = crc & 0x8000UL;

            crc = (crc << 1U);

            /* choose input bytes or implicit trailing zeros */
            if (i < len) {
                crc |= !!(src->at(i) & (0x80U >> b));
            }

            if (divide != 0U) {
                crc = crc ^ polynomial;
            }
        }
        ++i;
    }

    return crc;
}
