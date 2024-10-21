#include "error_lookup.h"
#include "ui_error_lookup.h"
#include "smp_group.h"

error_lookup::error_lookup(QWidget *parent, smp_group_array *groups) :
    QDialog(parent),
    ui(new Ui::error_lookup)
{
    smp_groups = groups;
    ui->setupUi(this);

    //Add default groups
    ui->combo_group->addItem(QString("%1 (OS)").arg(SMP_GROUP_ID_OS));
    ui->combo_group->addItem(QString("%1 (Img)").arg(SMP_GROUP_ID_IMG));
    ui->combo_group->addItem(QString("%1 (Stats)").arg(SMP_GROUP_ID_STATS));
    ui->combo_group->addItem(QString("%1 (Settings)").arg(SMP_GROUP_ID_SETTINGS));
    ui->combo_group->addItem(QString("%1 (FS)").arg(SMP_GROUP_ID_FS));
    ui->combo_group->addItem(QString("%1 (Shell)").arg(SMP_GROUP_ID_SHELL));
    ui->combo_group->addItem(QString("%1 (Enum)").arg(SMP_GROUP_ID_ENUM));
    ui->combo_group->addItem(QString("%1 (Zephyr)").arg(SMP_GROUP_ID_ZEPHYR));
}

error_lookup::~error_lookup()
{
    delete ui;
}

void error_lookup::on_button_lookup_clicked()
{
    smp_error_t error;
    bool converted;
    uint error_value;

    error_value = ui->edit_rc->text().toInt(&converted);

    if (converted == false)
    {
        ui->edit_error_description->appendPlainText("Supplied result code value is not a number.");
        return;
    }

    if (ui->radio_group_error_code->isChecked())
    {
        //SMP version 2 (group) error
        uint group_value;
        QString tmp;
        int space_position;
        QString error_define;
        QString error_string;

        //Check for a space, if found, extract value up to the first space and convert to a number
        space_position = ui->combo_group->currentText().indexOf(" ");

        if (space_position == -1)
        {
            tmp = ui->combo_group->currentText();
        }
        else
        {
            tmp = ui->combo_group->currentText().left(space_position);
        }

        group_value = tmp.toUInt(&converted);

        if (converted == false)
        {
            ui->edit_error_description->appendPlainText("Supplied group value is not a number.");
            return;
        }


        error.type = SMP_ERROR_RET;
        error.group = group_value;
        error.rc = error_value;
        error_define = smp_error::error_lookup_define(&error);
        error_string = smp_error::error_lookup_string(&error);

        ui->edit_error_id->setText(error_define);
        ui->edit_error_description->clear();
        ui->edit_error_description->appendPlainText(error_string);
    }
    else
    {
        //SMP error code
        error.type = SMP_ERROR_RC;
        error.rc = error_value;

        QString error_define = smp_error::error_lookup_define(&error);
        QString error_string = smp_error::error_lookup_string(&error);
        ui->edit_error_id->setText(error_define);
        ui->edit_error_description->clear();
        ui->edit_error_description->appendPlainText(error_string);
    }
}

void error_lookup::on_radio_group_error_code_toggled(bool checked)
{
    if (checked == true)
    {
        ui->combo_group->setEnabled(true);
    }
}

void error_lookup::on_radio_smp_error_code_toggled(bool checked)
{
    if (checked == true)
    {
        ui->combo_group->setEnabled(false);
    }
}
