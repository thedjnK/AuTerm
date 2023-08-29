#include "error_lookup.h"
#include "ui_error_lookup.h"

error_lookup::error_lookup(QWidget *parent, smp_group_array *groups) :
    QDialog(parent),
    ui(new Ui::error_lookup)
{
    smp_groups = groups;
    ui->setupUi(this);
}

error_lookup::~error_lookup()
{
    delete ui;
}

void error_lookup::on_button_lookup_clicked()
{

}

