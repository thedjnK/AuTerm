#ifndef ERROR_LOOKUP_H
#define ERROR_LOOKUP_H

#include <QDialog>
#include "smp_group_array.h"

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

private:
    Ui::error_lookup *ui;
    const smp_group_array *smp_groups;
};

#endif // ERROR_LOOKUP_H
