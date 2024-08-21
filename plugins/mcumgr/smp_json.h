#ifndef SMP_JSON_H
#define SMP_JSON_H

#include <QObject>
#include <QCborStreamReader>
#include "smp_message.h"

enum smp_logging_mode_t {
    SMP_LOGGING_MODE_JSON,
    SMP_LOGGING_MODE_YAML,
    SMP_LOGGING_MODE_CBOR,

    SMP_LOGGING_MODE_COUNT
};

class smp_json : public QObject
{
    Q_OBJECT

public:
    explicit smp_json(QObject *parent = nullptr);
    ~smp_json();
    void append_data(bool sent, smp_message *message);
    void set_indent(uint8_t indent);
    void set_mode(enum smp_logging_mode_t mode);

private slots:

signals:
    void log(bool sent, QString *data);

private:
    bool parse_message_json(QCborStreamReader &reader, QString *output, uint16_t indent, bool *outputs, bool list);
    bool parse_message_yaml(QCborStreamReader &reader, QString *output, uint16_t indent, bool *outputs, bool list);

    uint16_t indent_spaces;
    enum smp_logging_mode_t output_mode;
};

#endif // SMP_JSON_H
