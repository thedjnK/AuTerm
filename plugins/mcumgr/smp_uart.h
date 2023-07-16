/******************************************************************************
** Copyright (C) 2021-2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_uart.h
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
#ifndef smp_uart_H
#define smp_uart_H

#include <QObject>

class smp_uart : public QObject
{
	Q_OBJECT

	    public:
		     smp_uart(QObject *parent);
	~smp_uart();
	void send(QByteArray *data);
	int get_mtu();

private:
	void data_received(QByteArray message);

signals:
	void serial_write(QByteArray *data);
	void receive_waiting(QByteArray data);

public slots:
	void serial_read(QByteArray *rec_data);

private:
	QByteArray SerialData;
	QByteArray SMPBuffer;
	QByteArray SMPBufferActualData;
	bool SMPWaitingForContinuation = false;
	const QByteArray smp_first_header = QByteArrayLiteral("\x06\x09");
	const QByteArray smp_continuation_header = QByteArrayLiteral("\x04\x14");
	uint16_t waiting_packet_length = 0;
};

#endif // smp_uart_H
