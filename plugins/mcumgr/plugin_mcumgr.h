/******************************************************************************
** Copyright (C) 2021-2023 Jamie M.
**
** Project: AuTerm
**
** Module:  plugin_mcumgr.h
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
#ifndef PLUGIN_MCUMGR_H
#define PLUGIN_MCUMGR_H

#include <QObject>
#include <QtPlugin>
#include <QMainWindow>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include "AutPlugin.h"

//Form includes
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColumnView>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

class plugin_mcumgr : public QObject, AutPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AuTermPluginInterface_iid)
    Q_INTERFACES(AutPlugin)

public:
    QWidget *GetWidget();
    void setup(QMainWindow *main_window);
    const QString plugin_about();
    bool plugin_configuration();
    void serial_receive(QByteArray *data);
    void serial_error(QSerialPort::SerialPortError speErrorCode);
    void serial_bytes_written(qint64 intByteCount);
    void serial_about_to_close();
    void serial_opened();
    void serial_closed();

signals:
    void show_message_box(QString str_message);
    void plugin_set_status(bool busy, bool hide_terminal_output);

private slots:
    void on_btn_IMG_Local_clicked();
    void on_btn_IMG_Go_clicked();
    void receive_waiting(QByteArray data);
    bool eventFilter(QObject *object, QEvent *event);

private:
    bool handleStream_upload(QCborStreamReader &reader, int32_t *new_rc, int64_t *new_off);
	bool handleStream_state(QCborStreamReader &reader, int32_t *new_rc, QString array_name);
    void file_upload(QByteArray *message);
    bool extract_hash(QByteArray *file_data);

    //Form items
//    QGridLayout *gridLayout;
//    QTabWidget *tabWidget;
    QWidget *tab;
    QLabel *label;
    QTabWidget *tabWidget_2;
    QWidget *tab_2;
    QGridLayout *gridLayout_2;
    QLabel *lbl_FS_Status;
    QLabel *label_2;
    QProgressBar *progress_FS_Complete;
    QPlainTextEdit *edit_FS_Log;
    QLineEdit *edit_FS_Remote;
    QToolButton *btn_FS_Local;
    QLineEdit *edit_FS_Local;
    QLabel *label_3;
    QHBoxLayout *horizontalLayout;
    QRadioButton *radio_FS_Upload;
    QRadioButton *radio_FS_Download;
    QRadioButton *radio_FS_Size;
    QRadioButton *radio_FS_HashChecksum;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *btn_FS_Go;
    QSpacerItem *horizontalSpacer_2;
    QWidget *tab_3;
    QGridLayout *gridLayout_3;
    QPlainTextEdit *edit_IMG_Log;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *btn_IMG_Go;
    QSpacerItem *horizontalSpacer_4;
    QLabel *lbl_IMG_Status;
    QTabWidget *tabWidget_3;
    QWidget *tab_10;
    QGridLayout *gridLayout_4;
    QHBoxLayout *horizontalLayout_4;
    QSpinBox *edit_IMG_Image;
    QRadioButton *radio_IMG_No_Action;
    QRadioButton *radio_IMG_Test;
    QRadioButton *radio_IMG_Confirm;
    QHBoxLayout *horizontalLayout_5;
    QLineEdit *edit_IMG_Local;
    QToolButton *btn_IMG_Local;
    QLabel *label_4;
    QLabel *label_41;
    QProgressBar *progress_IMG_Complete;
    QLabel *label_6;
    QWidget *tab_9;
    QGridLayout *gridLayout_5;
    QColumnView *colview_IMG_Images;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_5;
    QRadioButton *radio_IMG_Get;
    QRadioButton *radio_ING_Set;
    QSpacerItem *horizontalSpacer_5;
    QWidget *tab_4;
    QWidget *tab_5;
    QWidget *tab_6;
    QSpinBox *edit_MTU;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label_7;
    QLineEdit *edit_IMG_Preview_Hash;
    QLabel *label_8;
    QLineEdit *edit_IMG_Preview_Version;
    QGridLayout *gridLayout_6;
    QCheckBox *check_IMG_Preview_Confirmed;
    QCheckBox *check_IMG_Preview_Active;
    QCheckBox *check_IMG_Preview_Pending;
    QCheckBox *check_IMG_Preview_Bootable;
    QCheckBox *check_IMG_Preview_Permanent;
    QPushButton *btn_IMG_Preview_Copy;
    QSpacerItem *verticalSpacer;

    //
    QByteArray file_upload_data;
    bool file_upload_in_progress;
    bool file_list_in_progress;
    uint32_t file_upload_area;
    QElapsedTimer upload_tmr;
    const QByteArray image_tlv_magic = QByteArrayLiteral("\x07\x69");
    QByteArray upload_hash;
};

#endif // PLUGIN_MCUMGR_H
