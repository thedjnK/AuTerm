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

#include "smp_uart.h"
#include "smp_group_img_mgmt.h"
#include "smp_group_os_mgmt.h"
#include "smp_group_shell_mgmt.h"
#include "smp_group_stat_mgmt.h"
#include "smp_group_fs_mgmt.h"
#include "smp_error.h"

//Form includes
///AUTOGEN_START_INCLUDES
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColumnView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
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
///AUTOGEN_END_INCLUDES

enum mcumgr_action_t {
    ACTION_IDLE,

    ACTION_IMG_UPLOAD,
    ACTION_IMG_UPLOAD_SET,
    ACTION_OS_UPLOAD_RESET,
    ACTION_IMG_IMAGE_LIST,
    ACTION_IMG_IMAGE_SET,
    ACTION_IMG_IMAGE_ERASE,

    ACTION_OS_ECHO,
    ACTION_OS_TASK_STATS,
    ACTION_OS_MEMORY_POOL,
    ACTION_OS_RESET,
    ACTION_OS_MCUMGR_BUFFER,
    ACTION_OS_OS_APPLICATION_INFO,

    ACTION_SHELL_EXECUTE,

    ACTION_STAT_GROUP_DATA,
    ACTION_STAT_LIST_GROUPS,

    ACTION_FS_UPLOAD,
    ACTION_FS_DOWNLOAD,
    ACTION_FS_STATUS,
    ACTION_FS_HASH_CHECKSUM,
    ACTION_FS_SUPPORTED_HASHES_CHECKSUMS,
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
    void on_btn_transport_connect_clicked();
    void on_colview_IMG_Images_updatePreviewWidget(const QModelIndex &index);

private:
    bool handleStream_shell(QCborStreamReader &reader, int32_t *new_rc, int32_t *new_ret, QString *new_data);
    smp_transport *active_transport();

    //Form items
///AUTOGEN_START_OBJECTS
//    QGridLayout *gridLayout;
//    QTabWidget *tabWidget;
    QWidget *tab;
    QTabWidget *tabWidget_2;
    QWidget *tab_FS;
    QGridLayout *gridLayout_2;
    QLineEdit *edit_FS_Local;
    QToolButton *btn_FS_Local;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *btn_FS_Go;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout;
    QRadioButton *radio_FS_Upload;
    QRadioButton *radio_FS_Download;
    QRadioButton *radio_FS_Size;
    QRadioButton *radio_FS_HashChecksum;
    QRadioButton *radio_FS_Hash_Checksum_Types;
    QLabel *lbl_FS_Status;
    QLabel *label_2;
    QPlainTextEdit *edit_FS_Log;
    QLabel *label_3;
    QProgressBar *progress_FS_Complete;
    QLineEdit *edit_FS_Remote;
    QLabel *label_19;
    QComboBox *combo_FS_Hash_Checksum;
    QWidget *tab_IMG;
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
    QRadioButton *radio_IMG_Set;
    QFrame *line;
    QCheckBox *check_IMG_Confirm;
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
    QGridLayout *gridLayout_14;
    QTableWidget *table_OS_Tasks;
    QWidget *tab_OS_Memory;
    QTableWidget *table_OS_Memory;
    QWidget *tab_OS_Reset;
    QGridLayout *gridLayout_12;
    QCheckBox *check_OS_Force_Reboot;
    QSpacerItem *verticalSpacer_3;
    QWidget *tab_OS_Info;
    QGridLayout *gridLayout_13;
    QLabel *label_17;
    QLineEdit *edit_OS_UName;
    QHBoxLayout *horizontalLayout_10;
    QRadioButton *radio_OS_Buffer_Info;
    QRadioButton *radio_OS_uname;
    QLabel *label_18;
    QPlainTextEdit *edit_OS_Info_Output;
    QHBoxLayout *horizontalLayout_13;
    QSpacerItem *horizontalSpacer_17;
    QPushButton *btn_OS_Go;
    QSpacerItem *horizontalSpacer_18;
    QLabel *lbl_OS_Status;
    QWidget *tab_Stats;
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
    QWidget *tab_Shell;
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
    QWidget *tab_Settings;
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
    QRadioButton *radio_transport_uart;
    QRadioButton *radio_transport_udp;
    QPushButton *btn_transport_connect;
    QSpacerItem *horizontalSpacer_6;
///AUTOGEN_END_OBJECTS

    //
    QByteArray upload_hash;
    mcumgr_action_t mode;
    int parent_row;
    int parent_column;
    int child_row;
    int child_column;
    QList<task_list_t> task_list;
    QList<memory_pool_t> memory_list;
    int32_t shell_rc;
    QStringList group_list;
    QList<stat_value_t> stat_list;
};

#endif // PLUGIN_MCUMGR_H
