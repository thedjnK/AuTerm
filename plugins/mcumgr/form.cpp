#include "form.h"
#include "ui_form.h"

Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
{
    ui->setupUi(this);
}

Form::~Form()
{
    delete ui;
}

void Form::on_btn_FS_Local_clicked()
{

}


void Form::on_btn_FS_Go_clicked()
{

}


void Form::on_radio_FS_Upload_clicked()
{

}


void Form::on_radio_FS_Download_clicked()
{

}


void Form::on_radio_FS_Size_clicked()
{

}


void Form::on_radio_FS_HashChecksum_clicked()
{

}


void Form::on_btn_IMG_Local_clicked()
{

}


void Form::on_btn_IMG_Go_clicked()
{

}

