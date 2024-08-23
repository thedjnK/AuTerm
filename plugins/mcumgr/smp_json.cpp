/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_json.cpp
**
** Notes:
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
#include "smp_json.h"

smp_json::smp_json(QObject *parent)
{
    indent_spaces = 4;
    output_mode = SMP_LOGGING_MODE_JSON;
}

smp_json::~smp_json()
{
}

bool smp_json::parse_message_json(QCborStreamReader &reader, QString *output, uint16_t indent, bool *outputs, bool list)
{
    //    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    bool keyset = true;
    bool keysetnow = false;
    bool finalarrayelement = false;

    while (!reader.lastError() && reader.hasNext())
    {
        keysetnow = false;
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;
        finalarrayelement = false;

        switch (reader.type())
        {
        case QCborStreamReader::UnsignedInteger:
        {
            if (list)
            {
                output->append(QString(" ").repeated(indent).append("\"").append(QString::number(reader.toUnsignedInteger())).append(",\n"));
            }
            else
            {
                output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": ").append(QString::number(reader.toUnsignedInteger())).append(",\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::NegativeInteger:
        {
            if (list)
            {
                output->append(QString(" ").repeated(indent).append("\"").append(QString::number((int)reader.toNegativeInteger())).append(",\n"));
            }
            else
            {
                output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": ").append(QString::number((int)reader.toNegativeInteger())).append(",\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::SimpleType:
        {
            QCborSimpleType data = reader.toSimpleType();

            if (list)
            {
                output->append(QString(" ").repeated(indent).append("\"").append(data == QCborSimpleType::False ? "false" : (data == QCborSimpleType::True ? "true" : "null")).append(",\n"));
            }
            else
            {
                output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": ").append(data == QCborSimpleType::False ? "false" : (data == QCborSimpleType::True ? "true" : "null")).append(",\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::Float16:
        {
            if (list)
            {
                output->append(QString(" ").repeated(indent).append("\"").append(QString::number((double)reader.toFloat16())).append(",\n"));
            }
            else
            {
                output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": ").append(QString::number((double)reader.toFloat16())).append(",\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::Float:
        {
            if (list)
            {
                output->append(QString(" ").repeated(indent).append("\"").append(QString::number(reader.toFloat())).append(",\n"));
            }
            else
            {
                output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": ").append(QString::number(reader.toFloat())).append(",\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::Double:
        {
            if (list)
            {
                output->append(QString(" ").repeated(indent).append("\"").append(QString::number(reader.toDouble())).append(",\n"));
            }
            else
            {
                output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": ").append(QString::number(reader.toDouble())).append(",\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::ByteArray:
        {
            QByteArray data;
            auto r = reader.readByteArray();
            while (r.status == QCborStreamReader::Ok)
            {
                data.append(r.data);
                r = reader.readByteArray();
            }

            if (r.status == QCborStreamReader::Error)
            {
                data.clear();
//                ui->edit_output->append("error byte");
//                log_error() << "Error decoding byte array";
            }
            else
            {
                if (list)
                {
                    output->append(QString(" ").repeated(indent).append("\"0x").append(data.toHex()).append("\"").append(",\n"));
                }
                else
                {
                    output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": \"0x").append(data.toHex()).append("\"").append(",\n"));
                }

                if (outputs != nullptr)
                {
                    *outputs = true;
                }
            }

            break;
        }
        case QCborStreamReader::String:
        {
            QString data;
            auto r = reader.readString();
            while (r.status == QCborStreamReader::Ok)
            {
                data.append(r.data);
                r = reader.readString();
            }

            if (r.status == QCborStreamReader::Error)
            {
                data.clear();
//                ui->edit_output->append("error string");
//                log_error() << "Error decoding string";
            }
            else
            {
                if (key.isEmpty() && list == false)
                {
                    key = data;
                    keyset = true;
                }
                else
                {
                    data = data.replace("\r", "\\r").replace("\n", "\\n");
                    if (list)
                    {
                        output->append(QString(" ").repeated(indent).append("\"").append(data).append("\",\n"));
                    }
                    else
                    {
                        output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": \"").append(data).append("\"").append(",\n"));
                    }

                    if (outputs != nullptr)
                    {
                        *outputs = true;
                    }
                }
            }

            break;
        }
        case QCborStreamReader::Array:
        case QCborStreamReader::Map:
        {
            QCborStreamReader::Type array_map_type = reader.type();
            if (array_map_type == QCborStreamReader::Array)
            {
                if (key.isEmpty())
                {
                    output->append(QString(" ").repeated(indent).append("[\n"));
                }
                else
                {
                    output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": [\n"));
                }
            }
            else
            {
                if (key.isEmpty())
                {
                    output->append(QString(" ").repeated(indent).append("{\n"));
                }
                else
                {
                    output->append(QString(" ").repeated(indent).append("\"").append(key).append("\": {\n"));
                }
            }

            reader.enterContainer();

            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                bool has_outputs = false;

                parse_message_json(reader, output, (indent + indent_spaces), &has_outputs, (array_map_type == QCborStreamReader::Array));

                if (has_outputs == true)
                {
                    //Remove the last element's comma
                    output->remove((output->length() - 2), 1);
                }
            }

            if (reader.lastError() == QCborError::NoError)
            {
                reader.leaveContainer();
            }

            if (array_map_type == QCborStreamReader::Array)
            {
                output->append(QString(" ").repeated(indent).append("],\n"));
                finalarrayelement = true;

                //Set outputs to false if this is present, this is because it will be removed by the parent loop if it's the last element
                if (outputs != nullptr)
                {
                    *outputs = false;
                }
            }
            else
            {
                output->append(QString(" ").repeated(indent).append("},\n"));
                finalarrayelement = true;

                //Set outputs to false if this is present, this is because it will be removed by the parent loop if it's the last element
                if (outputs != nullptr)
                {
                    *outputs = false;
                }
            }

            if (!key.isEmpty())
            {
                key.clear();
            }

            break;
        }
        default:
        {
            reader.next();
            continue;
        }
        };

        if (keysetnow == false && keyset == true)
        {
            keysetnow = true;
        }
    }

    if (!key.isEmpty() && keysetnow == true)
    {
        output->append(QString(" ").repeated(indent).append("\"").append(key).append("\""));
    }

    if (finalarrayelement == true)
    {
        //Remove the last element's comma
        output->remove((output->length() - 2), 1);
    }

    return true;
}

bool smp_json::parse_message_yaml(QCborStreamReader &reader, QString *output, uint16_t indent, bool *outputs, bool list, bool first_entry)
{
    //    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    bool keyset = true;
    bool keysetnow = false;

    while (!reader.lastError() && reader.hasNext())
    {
        keysetnow = false;
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;

        switch (reader.type())
        {
        case QCborStreamReader::UnsignedInteger:
        {
            if (list || first_entry)
            {
                output->append(QString(" ").repeated(indent - 2).append("- \"").append(QString::number(reader.toUnsignedInteger())).append("\n"));
                first_entry = false;
            }
            else
            {
                output->append(QString(" ").append(QString::number(reader.toUnsignedInteger())).append("\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::NegativeInteger:
        {
            if (list || first_entry)
            {
                output->append(QString(" ").repeated(indent - 2).append("- \"").append(QString::number((int)reader.toNegativeInteger())).append("\n"));
                first_entry = false;
            }
            else
            {
                output->append(QString(" ").append(QString::number((int)reader.toNegativeInteger())).append("\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::SimpleType:
        {
            QCborSimpleType data = reader.toSimpleType();

            if (list || first_entry)
            {
                output->append(QString(" ").repeated(indent - 2).append("- \"").append(data == QCborSimpleType::False ? "false" : (data == QCborSimpleType::True ? "true" : "null")).append("\n"));
                first_entry = false;
            }
            else
            {
                output->append(QString(" ").append((data == QCborSimpleType::False ? QString("false") : (data == QCborSimpleType::True ? QString("true") : QString("")))).append("\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::Float16:
        {
            if (list || first_entry)
            {
                output->append(QString(" ").repeated(indent - 2).append("- \"").append(QString::number((double)reader.toFloat16())).append("\n"));
                first_entry = false;
            }
            else
            {
                output->append(QString(" ").append(QString::number((double)reader.toFloat16())).append("\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::Float:
        {
            if (list || first_entry)
            {
                output->append(QString(" ").repeated(indent - 2).append("- \"").append(QString::number(reader.toFloat())).append("\n"));
                first_entry = false;
            }
            else
            {
                output->append(QString(" ").append(QString::number(reader.toFloat())).append("\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::Double:
        {
            if (list || first_entry)
            {
                output->append(QString(" ").repeated(indent - 2).append("- \"").append(QString::number(reader.toDouble())).append("\n"));
                first_entry = false;
            }
            else
            {
                output->append(QString(" ").append(QString::number(reader.toDouble())).append("\n"));
            }

            if (outputs != nullptr)
            {
                *outputs = true;
            }

            reader.next();
            break;
        }
        case QCborStreamReader::ByteArray:
        {
            QByteArray data;
            auto r = reader.readByteArray();
            while (r.status == QCborStreamReader::Ok)
            {
                data.append(r.data);
                r = reader.readByteArray();
            }

            if (r.status == QCborStreamReader::Error)
            {
                data.clear();
                //                ui->edit_output->append("error byte");
                //                log_error() << "Error decoding byte array";
            }
            else
            {
                if (list || first_entry)
                {
                    output->append(QString(" ").repeated(indent - 2).append("- \"0x").append(data.toHex()).append("\"").append("\n"));
                    first_entry = false;
                }
                else
                {
                    output->append(QString(" ").append("\"0x").append(data.toHex()).append("\"").append("\n"));
                }

                if (outputs != nullptr)
                {
                    *outputs = true;
                }
            }

            break;
        }
        case QCborStreamReader::String:
        {
            QString data;
            auto r = reader.readString();
            while (r.status == QCborStreamReader::Ok)
            {
                data.append(r.data);
                r = reader.readString();
            }

            if (r.status == QCborStreamReader::Error)
            {
                data.clear();
                //                ui->edit_output->append("error string");
                //                log_error() << "Error decoding string";
            }
            else
            {
                if (key.isEmpty() && list == false)
                {
                    key = data;
                    if (first_entry)
                    {
                        output->append(QString(" ").repeated(indent - 2).append("- ").append(data).append(":"));
                        first_entry = false;
                    }
                    else
                    {
                        output->append(QString(" ").repeated(indent).append("").append(data).append(":"));
                    }

                    keyset = true;
                }
                else
                {
                    //TODO: proper newline handling with |- style output
                    data = data.replace("\r", "\\r").replace("\n", "\\n");
                    if (list || first_entry)
                    {
                        output->append(QString(" ").repeated(indent - 2).append("- ").append(data).append("\n"));
                        first_entry = false;
                    }
                    else
                    {
                        output->append(QString(" ").append(data).append("\n"));
                    }

                    if (outputs != nullptr)
                    {
                        *outputs = true;
                    }
                }
            }

            break;
        }
        case QCborStreamReader::Array:
        case QCborStreamReader::Map:
        {
            QCborStreamReader::Type array_map_type = reader.type();

            reader.enterContainer();

            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                bool has_outputs = false;

                if (!key.isEmpty())
                {
                    output->append(QString("\n"));
                }

                parse_message_yaml(reader, output, (outputs == nullptr ? 0 : (indent + indent_spaces)), &has_outputs, (array_map_type == QCborStreamReader::Array), (list == true && array_map_type == QCborStreamReader::Map ? true : false));
            }

            if (reader.lastError() == QCborError::NoError)
            {
                reader.leaveContainer();
            }

            if (!key.isEmpty())
            {
                key.clear();
            }

            break;
        }
        default:
        {
            reader.next();
            continue;
        }
        };

        if (keysetnow == false && keyset == true)
        {
            keysetnow = true;
        }
    }

    if (!key.isEmpty() && keysetnow == true)
    {
        output->append(QString(" ").repeated(indent).append("\"").append(key).append("\""));
    }

    return true;
}

void smp_json::append_data(bool sent, smp_message *message)
{
    QString output_data;

    if (output_mode == SMP_LOGGING_MODE_JSON)
    {
        QCborStreamReader reader(message->contents());
        (void)parse_message_json(reader, &output_data, 0, nullptr, false);
    }
    else if (output_mode == SMP_LOGGING_MODE_YAML)
    {
        QCborStreamReader reader(message->contents());
        (void)parse_message_yaml(reader, &output_data, 0, nullptr, false, false);
    }
    else if (output_mode == SMP_LOGGING_MODE_CBOR)
    {
        output_data = message->contents().toHex();
    }

    emit log(sent, &output_data);
}

void smp_json::set_indent(uint8_t indent)
{
    indent_spaces = (uint16_t)indent;
}

void smp_json::set_mode(enum smp_logging_mode_t mode)
{
    output_mode = mode;
}
