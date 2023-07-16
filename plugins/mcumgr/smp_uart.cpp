/******************************************************************************
** Copyright (C) 2021-2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_uart.cpp
**
** Notes:   With exception to the crc16() function which is apache 2.0 licensed
**
** License: This program is free software: you can redistribute it and/or
**          modify it under the terms of the GNU General Public License as
**          published by the Free Software Foundation, version 3.
**
**          This program is distributed in the hope that it will be useful,
**          but WITHOUT ANY WARRANTY; without even the implied warranty of
**          MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**          GNU General Public License for more details.
**
**          You should have received a copy of the GNU General Public License
**          along with this program.  If not, see http://www.gnu.org/licenses/
**
*******************************************************************************/
#include "smp_uart.h"
#include <QDebug>

//TODO: all of this

uint16_t crc16(QByteArray *src, size_t i, size_t len, uint16_t polynomial,
	       uint16_t initial_value, bool pad);

smp_uart::smp_uart(QObject *parent)
{
}

smp_uart::~smp_uart()
{
}

static QByteArray somebuffer;

void smp_uart::data_received(QByteArray message)
{
	somebuffer.append(message);
	if (somebuffer.length() == 0)
	{
		qDebug() << "Failed decoding base64";
	}
	else if (somebuffer.length() >= 8)
	{
		uint16_t len = somebuffer.at(2);
		len <<= 8;
		len |= somebuffer.at(3);

		if (somebuffer.length() >= (len + 8))
		{
			emit receive_waiting(somebuffer);
			somebuffer.clear();
		}
	}
}

void smp_uart::serial_read(QByteArray *rec_data)
{
	SerialData.append(*rec_data);
//	qDebug() << QString("Now: ").append(SerialData);

	//Search for SMP packets
	int32_t pos = SerialData.indexOf(smp_first_header);
	int32_t pos_other = SerialData.indexOf(smp_continuation_header);
	int32_t posA = SerialData.indexOf(0x0a, pos + 2);
	int32_t posA_other = SerialData.indexOf(0x0a, pos_other + 2);

	while ((pos != -1 && posA != -1) || (pos_other != -1 && posA_other != -1))
										   //        while (pos != -1 && posA != -1 /*&& SMPWaitingForContinuation == false*/)
	{
		if (pos >= 0 && (pos_other == -1 || pos < pos_other))
		{
			//Start
			//Check this header
			SMPBuffer.clear();
//			qDebug() << SerialData.mid((pos + 2), (posA - pos - 2));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
			SMPBuffer = QByteArray::fromBase64(SerialData.mid((pos + 2), (posA - pos - 2)), QByteArray::AbortOnBase64DecodingErrors);
#else
			SMPBuffer = QByteArray::fromBase64(SerialData.mid((pos + 2), (posA - pos - 2)));
#endif

			if (SMPBuffer.length() == 0)
			{
				qDebug() << "Failed decoding base64";
			}
			else if (SMPBuffer.length() > 2)
			{
//				qDebug() << "length here is " << SMPBuffer.length();
				//Check length
				waiting_packet_length = ((uint16_t)SMPBuffer[0]) << 8;
				waiting_packet_length |= ((uint16_t)SMPBuffer[1] & 0xff);
				SMPBuffer.remove(0, 2);
				if (SMPBuffer.length() >= (waiting_packet_length))
				{
					//We have a full packet, check the checksum
					uint16_t crc = crc16(&SMPBuffer, 0, SMPBuffer.length() - 2, 0x1021, 0, true);
					uint16_t message_crc = ((uint16_t)SMPBuffer[(SMPBuffer.length() - 2)]) << 8;
					message_crc |= SMPBuffer[(SMPBuffer.length() - 1)] & 0xff;
					if (crc == message_crc)
					{
						//Good to parse message after removing CRC
						SMPBuffer.remove((SMPBuffer.length() - 2), 2);
						emit receive_waiting(SMPBuffer);
					}
					else
					{
						//CRC failure
						qDebug() << "CRC failure, expected " << message_crc << " but got " << crc;
					}
				}
				else
				{
					//More data expected in another packet
					SMPWaitingForContinuation = true;
					SMPBufferActualData = SMPBuffer;
				}

				SerialData.remove(pos, (posA - pos + 1));

				pos = SerialData.indexOf(smp_first_header);
				posA = SerialData.indexOf(0x0a, pos + 2);
				pos_other = SerialData.indexOf(smp_continuation_header);
				posA_other = SerialData.indexOf(0x0a, pos_other + 2);
			}
		}
		else if (SMPWaitingForContinuation == true)
		{
			//Continuation
			//Check this header
			SMPBuffer.clear();
//			qDebug() << SerialData.mid((pos_other + 2), (posA_other - pos_other - 2));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
			SMPBuffer = QByteArray::fromBase64(SerialData.mid((pos_other + 2), (posA_other - pos_other - 2)), QByteArray::AbortOnBase64DecodingErrors);
#else
			SMPBuffer = QByteArray::fromBase64(SerialData.mid((pos_other + 2), (posA_other - pos_other - 2)));
#endif

			if (SMPBuffer.length() == 0)
			{
				qDebug() << "Failed decoding base64";
			}
			else if (SMPBuffer.length() > 2)
			{
				//Check length
				SMPBufferActualData.append(SMPBuffer);
				if (SMPBufferActualData.length() >= (waiting_packet_length /*+ 2*/))
				{
					//We have a full packet, check the checksum
					uint16_t crc = crc16(&SMPBufferActualData, 0, SMPBufferActualData.length() - 2, 0x1021, 0, true);
					uint16_t message_crc = ((uint16_t)SMPBufferActualData[(SMPBufferActualData.length() - 2)]) << 8;
					message_crc |= SMPBufferActualData[(SMPBufferActualData.length() - 1)] & 0xff;
					if (crc == message_crc)
					{
						//Good to parse message after removing CRC
						SMPBufferActualData.remove((SMPBufferActualData.length() - 2), 2);
						emit receive_waiting(SMPBufferActualData);
					}
					else
					{
						//CRC failure
						qDebug() << "CRC failure, expected " << message_crc << " but got " << crc;
					}

					SMPBufferActualData.clear();
					SMPWaitingForContinuation = false;
				}
				else
				{
					//More data expected in another packet
					SMPWaitingForContinuation = true;
				}

				SerialData.remove(pos_other, (posA_other - pos_other + 1));

				pos = SerialData.indexOf(smp_first_header);
				if (pos == -1)
				{
					posA = -1;
				}
				else
				{
					posA = SerialData.indexOf(0x0a, pos + 2);
				}
				pos_other = SerialData.indexOf(smp_continuation_header);
				if (pos_other == -1)
				{
					posA_other = -1;
				}
				else
				{
					posA_other = SerialData.indexOf(0x0a, pos_other + 2);
				}
			}
		}
	}

	if (SerialData.length() > 10 && SerialData.indexOf(smp_first_header) == -1 && SerialData.indexOf(smp_continuation_header) == -1)
	{
		qDebug() << "Clearing garbage data";
//		qDebug() << SerialData;
		SerialData.clear();
	}
}

//Taken from zephyr - Apache 2.0 licensed
uint16_t crc16(QByteArray *src, size_t i, size_t len, uint16_t polynomial,
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

void smp_uart::send(QByteArray *message)
{
	//127 bytes = 3 + base 64 message
	//base64 = 4 bytes output per 3 byte input
	QByteArray output;
	uint16_t size = message->length();
	size += 2;
	output.append((uint8_t)((size & 0xff00) >> 8));
	output.append((uint8_t)(size & 0xff));
	uint16_t crc = crc16(message, 0, message->length(), 0x1021, 0, true);

	QByteArray inbase;
	inbase.append(smp_first_header);

	while (((message->length() * 4) / 3) + 3 >= 127)
	{
		/* Chunking required */
		uint8_t chunk_size = 93 - output.length();

		output.append(message->left(chunk_size));
		message->remove(0, chunk_size);
		inbase.append(output.toBase64());
		inbase.append((uint8_t)0x0a);
		emit serial_write(&inbase);
//		qDebug() << "out: " << inbase  << " -> " << inbase.length() << "chunk_size = " << chunk_size << "output: " << output << " in b64: " << output.toBase64();

		inbase.clear();
		inbase.append(smp_continuation_header);
		output.clear();
	}
	output.append(*message);

	output.append((uint8_t)((crc & 0xff00) >> 8));
	output.append((uint8_t)(crc & 0xff));

	inbase.append(output.toBase64());
	inbase.append((uint8_t)0x0a);

#if 0
    qDebug() << "Start: ";
    uint8_t i = 0;
    while (i < inbase.length())
    {
	qDebug() << (uint8_t)inbase.at(i);
	++i;
    }
    qDebug() << "End";
#endif

	emit serial_write(&inbase);
//	qDebug() << "out: " << inbase  << " -> " << inbase.length();
}
