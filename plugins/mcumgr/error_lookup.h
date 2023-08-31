#ifndef ERROR_LOOKUP_H
#define ERROR_LOOKUP_H

#include <QDialog>
#include "smp_group_array.h"
#include "smp_error.h"

namespace Ui {
    class error_lookup;
}

class error_lookup : public QDialog
{
    Q_OBJECT

public:
    explicit error_lookup(QWidget *parent = nullptr, smp_group_array *groups = nullptr);
    ~error_lookup();

private slots:
    void on_button_lookup_clicked();
    void on_radio_group_error_code_toggled(bool checked);
    void on_radio_smp_error_code_toggled(bool checked);

private:
    Ui::error_lookup *ui;
    const smp_group_array *smp_groups;

};

#endif // ERROR_LOOKUP_H
