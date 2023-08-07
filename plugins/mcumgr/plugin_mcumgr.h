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
#include "smp_processor.h"
#include "smp_group.h"

//Form includes
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColumnView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

enum mcumgr_action_t {
    ACTION_IMG_UPLOAD,
    ACTION_IMG_UPLOAD_SET,
    ACTION_IMG_IMAGE_LIST,
    ACTION_IMG_IMAGE_SET,
    ACTION_IMG_IMAGE_ERASE,

    ACTION_OS_ECHO,
};

class plugin_mcumgr : public QObject, AutPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AuTermPluginInterface_iid FILE "plugin_mcumgr.json")
    Q_INTERFACES(AutPlugin)

public:
    ~plugin_mcumgr();
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
    void plugin_add_open_close_button(QPushButton *button);
    void plugin_to_hex(QByteArray *data);

private slots:
    void receive_waiting(QByteArray data);
    bool eventFilter(QObject *object, QEvent *event);
    void group_to_hex(QByteArray *data);

    void status(uint8_t user_data, group_status status, QString error_string);
    void progress(uint8_t user_data, uint8_t percent);

    //Form slots
    void on_btn_FS_Local_clicked();
    void on_btn_FS_Go_clicked();
    void on_radio_FS_Upload_clicked();
    void on_radio_FS_Download_clicked();
    void on_radio_FS_Size_clicked();
    void on_radio_FS_HashChecksum_clicked();
    void on_btn_IMG_Local_clicked();
    void on_btn_IMG_Go_clicked();
    void on_radio_IMG_No_Action_toggled(bool checked);
    void on_btn_IMG_Preview_Copy_clicked();
    void on_btn_OS_Go_clicked();
    void on_btn_SHELL_Go_clicked();
    void on_btn_STAT_Go_clicked();
    void on_btn_SHELL_Clear_clicked();
    void on_btn_SHELL_Copy_clicked();
    void on_colview_IMG_Images_updatePreviewWidget(const QModelIndex &index);

private:
    bool handleStream_shell(QCborStreamReader &reader, int32_t *new_rc, int32_t *new_ret, QString *new_data);

    //Form items
//    QGridLayout *gridLayout;
//    QTabWidget *tabWidget;
    QWidget *tab;
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
    QWidget *tab_IMG_Images_2;
    QGridLayout *gridLayout_3;
    QPlainTextEdit *edit_IMG_Log;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *btn_IMG_Go;
    QSpacerItem *horizontalSpacer_4;
    QLabel *lbl_IMG_Status;
    QTabWidget *tabWidget_3;
    QWidget *tab_IMG_Upload;
    QGridLayout *gridLayout_4;
    QProgressBar *progress_IMG_Complete;
    QLabel *label_6;
    QHBoxLayout *horizontalLayout_5;
    QLineEdit *edit_IMG_Local;
    QToolButton *btn_IMG_Local;
    QLabel *label_4;
    QHBoxLayout *horizontalLayout_4;
    QSpinBox *edit_IMG_Image;
    QRadioButton *radio_IMG_No_Action;
    QRadioButton *radio_IMG_Test;
    QRadioButton *radio_IMG_Confirm;
    QLabel *label_41;
    QLabel *label_9;
    QCheckBox *check_IMG_Reset;
    QWidget *tab_IMG_Images;
    QGridLayout *gridLayout_5;
    QColumnView *colview_IMG_Images;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_5;
    QRadioButton *radio_IMG_Get;
    QRadioButton *radio_ING_Set;
    QSpacerItem *horizontalSpacer_5;
    QWidget *tab_IMG_Erase;
    QGridLayout *gridLayout_10;
    QLabel *label_14;
    QSpacerItem *horizontalSpacer_9;
    QSpinBox *edit_IMG_Erase_Slot;
    QSpacerItem *verticalSpacer_2;
    QWidget *tab_OS;
    QGridLayout *gridLayout_7;
    QTabWidget *selector_OS;
    QWidget *tab_OS_Echo;
    QGridLayout *gridLayout_8;
    QLabel *label_10;
    QPlainTextEdit *edit_OS_Echo_Input;
    QLabel *label_11;
    QPlainTextEdit *edit_OS_Echo_Output;
    QWidget *tab_OS_Tasks;
    QWidget *tab_OS_Memory;
    QWidget *tab_OS_Reset;
    QGridLayout *gridLayout_12;
    QCheckBox *check_OS_Force_Reboot;
    QSpacerItem *verticalSpacer_3;
    QWidget *tab_OS_Info;
    QGridLayout *gridLayout_13;
    QLabel *label_17;
    QLineEdit *edit_OS_UNam;
    QHBoxLayout *horizontalLayout_10;
    QRadioButton *radio_FS_Upload_2;
    QRadioButton *radio_FS_Download_2;
    QLabel *label_18;
    QPlainTextEdit *edit_OS_Info_Output;
    QHBoxLayout *horizontalLayout_13;
    QSpacerItem *horizontalSpacer_17;
    QPushButton *btn_OS_Go;
    QSpacerItem *horizontalSpacer_18;
    QLabel *lbl_OS_Status;
    QWidget *tab_5;
    QGridLayout *gridLayout_11;
    QLabel *label_15;
    QComboBox *combo_STAT_Group;
    QLabel *label_16;
    QTableWidget *table_STAT_Values;
    QHBoxLayout *horizontalLayout_9;
    QRadioButton *radio_STAT_List;
    QRadioButton *radio_STAT_Fetch;
    QLabel *lbl_STAT_Status;
    QHBoxLayout *horizontalLayout_14;
    QSpacerItem *horizontalSpacer_19;
    QPushButton *btn_STAT_Go;
    QSpacerItem *horizontalSpacer_20;
    QWidget *tab_6;
    QGridLayout *gridLayout_9;
    QVBoxLayout *verticalLayout_3;
    QToolButton *btn_SHELL_Clear;
    QToolButton *btn_SHELL_Copy;
    QLabel *lbl_SHELL_Status;
    QPlainTextEdit *edit_SHELL_Output;
    QLineEdit *edit_SHELL_Input;
    QLabel *label_12;
    QHBoxLayout *horizontalLayout_8;
    QSpacerItem *horizontalSpacer_7;
    QPushButton *btn_SHELL_Go;
    QSpacerItem *horizontalSpacer_8;
    QLabel *label_13;
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
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label;
    QSpinBox *edit_MTU;
    QCheckBox *check_V2_Protocol;
    QSpacerItem *horizontalSpacer_6;

    //
    QByteArray file_upload_data;
    bool file_upload_in_progress;
    bool file_list_in_progress;
    uint32_t file_upload_area;
    QElapsedTimer upload_tmr;
    const QByteArray image_tlv_magic = QByteArrayLiteral("\x07\x69");
    QByteArray upload_hash;
    bool shell_in_progress;
};

#endif // PLUGIN_MCUMGR_H
