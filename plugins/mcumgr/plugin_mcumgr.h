/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
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
#include "smp_uart_auterm.h"
#include "smp_group_fs_mgmt.h"
#include "smp_group_img_mgmt.h"
#include "smp_group_os_mgmt.h"
#include "smp_group_settings_mgmt.h"
#include "smp_group_shell_mgmt.h"
#include "smp_group_stat_mgmt.h"
#include "smp_error.h"
#include "smp_group_array.h"
#include "error_lookup.h"
#include "debug_logger.h"
#include "smp_json.h"

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
#include "smp_udp.h"
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
#include "smp_bluetooth.h"
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
#include "smp_lorawan.h"
#endif

//Form includes
///AUTOGEN_START_INCLUDES
#include <QtCore/QDate>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColumnView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateTimeEdit>
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
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "AutScrollEdit.h"
///AUTOGEN_END_INCLUDES

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum mcumgr_action_t {
    ACTION_IDLE,

    ACTION_IMG_UPLOAD,
    ACTION_IMG_UPLOAD_SET,
    ACTION_OS_UPLOAD_RESET,
    ACTION_IMG_IMAGE_LIST,
    ACTION_IMG_IMAGE_SET,
    ACTION_IMG_IMAGE_ERASE,
    ACTION_IMG_IMAGE_SLOT_INFO,

    ACTION_OS_ECHO,
    ACTION_OS_TASK_STATS,
    ACTION_OS_MEMORY_POOL,
    ACTION_OS_RESET,
    ACTION_OS_DATETIME_GET,
    ACTION_OS_DATETIME_SET,
    ACTION_OS_MCUMGR_BUFFER,
    ACTION_OS_OS_APPLICATION_INFO,
    ACTION_OS_BOOTLOADER_INFO,

    ACTION_SHELL_EXECUTE,

    ACTION_STAT_GROUP_DATA,
    ACTION_STAT_LIST_GROUPS,

    ACTION_FS_UPLOAD,
    ACTION_FS_DOWNLOAD,
    ACTION_FS_STATUS,
    ACTION_FS_HASH_CHECKSUM,
    ACTION_FS_SUPPORTED_HASHES_CHECKSUMS,

    ACTION_SETTINGS_READ,
    ACTION_SETTINGS_WRITE,
    ACTION_SETTINGS_DELETE,
    ACTION_SETTINGS_COMMIT,
    ACTION_SETTINGS_LOAD,
    ACTION_SETTINGS_SAVE,

    ACTION_ZEPHYR_STORAGE_ERASE,

    ACTION_ENUM_COUNT,
    ACTION_ENUM_LIST,
    ACTION_ENUM_SINGLE,
    ACTION_ENUM_DETAILS,

    ACTION_CUSTOM,
};

class plugin_mcumgr : public QObject, AutPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AuTermPluginInterface_iid FILE "plugin_mcumgr.json")
    Q_INTERFACES(AutPlugin)

public:
    ~plugin_mcumgr();
    void setup(QMainWindow *main_window) override;
    const QString plugin_about() override;
    bool plugin_configuration() override;
    static QMainWindow *get_main_window();
    void setup_finished() override;
    PluginType plugin_type() override;
    QObject *plugin_object() override;

signals:
    void show_message_box(QString str_message);
    void plugin_set_status(bool busy, bool hide_terminal_output, bool *accepted);
    void plugin_add_open_close_button(QPushButton *button);
    void plugin_to_hex(QByteArray *data);
    void plugin_serial_open_close(uint8_t mode);
    void plugin_serial_is_open(bool *open);

private slots:
    void serial_receive(QByteArray *data);
    void serial_error(QSerialPort::SerialPortError speErrorCode);
    void serial_bytes_written(qint64 intByteCount);
    void serial_about_to_close();
    void serial_opened();
    void serial_closed();
    bool eventFilter(QObject *object, QEvent *event) override;
    void group_to_hex(QByteArray *data);

    void status(uint8_t user_data, group_status status, QString error_string);
    void progress(uint8_t user_data, uint8_t percent);

    void enter_pressed();

    void custom_log(bool sent, QString *data);
    void custom_message_callback(enum custom_message_callback_t type, smp_error_t *data);

    //Form slots
    void on_btn_FS_Local_clicked();
    void on_btn_FS_Go_clicked();
    void on_radio_FS_Upload_toggled(bool checked);
    void on_radio_FS_Download_toggled(bool checked);
    void on_radio_FS_Size_toggled(bool checked);
    void on_radio_FS_HashChecksum_toggled(bool checked);
    void on_radio_FS_Hash_Checksum_Types_toggled(bool checked);
    void on_btn_IMG_Local_clicked();
    void on_btn_IMG_Go_clicked();
    void on_radio_IMG_No_Action_toggled(bool checked);
    void on_btn_IMG_Preview_Copy_clicked();
    void on_btn_OS_Go_clicked();
    void on_btn_STAT_Go_clicked();
    void on_btn_SHELL_Clear_clicked();
    void on_btn_SHELL_Copy_clicked();
    void on_btn_transport_connect_clicked();
    void on_colview_IMG_Images_updatePreviewWidget(const QModelIndex &index);
    void on_radio_transport_uart_toggled(bool checked);
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    void on_radio_transport_udp_toggled(bool checked);
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    void on_radio_transport_bluetooth_toggled(bool checked);
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    void on_radio_transport_lora_toggled(bool checked);
#endif
    void on_radio_OS_Buffer_Info_toggled(bool checked);
    void on_radio_OS_uname_toggled(bool checked);
    void on_radio_IMG_Get_toggled(bool checked);
    void on_radio_IMG_Set_toggled(bool checked);
    void on_radio_settings_read_toggled(bool checked);
    void on_radio_settings_write_toggled(bool checked);
    void on_radio_settings_delete_toggled(bool checked);
    void on_radio_settings_commit_toggled(bool checked);
    void on_radio_settings_load_toggled(bool checked);
    void on_radio_settings_save_toggled(bool checked);
    void on_btn_settings_go_clicked();
    void on_radio_settings_none_toggled(bool checked);
    void on_radio_settings_text_toggled(bool checked);
    void on_radio_settings_decimal_toggled(bool checked);
    void on_check_settings_big_endian_toggled(bool checked);
    void on_check_settings_signed_decimal_value_toggled(bool checked);
    void on_btn_zephyr_go_clicked();
    void on_check_os_datetime_use_pc_date_time_toggled(bool checked);
    void on_radio_os_datetime_get_toggled(bool checked);
    void on_radio_os_datetime_set_toggled(bool checked);
    void on_btn_enum_go_clicked();
    void on_radio_custom_custom_toggled(bool checked);
    void on_radio_custom_logging_toggled(bool checked);
    void on_radio_custom_json_toggled(bool checked);
    void on_radio_custom_yaml_toggled(bool checked);
    void on_radio_custom_cbor_toggled(bool checked);
    void on_btn_custom_copy_send_clicked();
    void on_btn_custom_copy_receive_clicked();
    void on_btn_custom_copy_both_clicked();
    void on_btn_custom_clear_clicked();
    void on_edit_custom_indent_valueChanged(int value);
    void on_btn_custom_go_clicked();
    void on_tree_IMG_Slot_Info_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_btn_error_lookup_clicked();
    void on_btn_cancel_clicked();

private:
    bool handleStream_shell(QCborStreamReader &reader, int32_t *new_rc, int32_t *new_ret, QString *new_data);
    smp_transport *active_transport();
    bool claim_transport(QLabel *status);
    void relase_transport(void);
    void flip_endian(uint8_t *data, uint8_t size);
    bool update_settings_display();
    void show_transport_open_status();
    void size_abbreviation(uint32_t size, QString *output);
    void close_transport_windows();
    void set_group_transport_settings(smp_group *group);
    void set_group_transport_settings(smp_group *group, uint32_t timeout);
    void update_img_state_table();

    //Form items
///AUTOGEN_START_OBJECTS
//    QGridLayout *gridLayout;
//    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label;
    QSpinBox *edit_MTU;
    QFrame *line_9;
    QCheckBox *check_V2_Protocol;
    QFrame *line_8;
    QRadioButton *radio_transport_uart;
    QRadioButton *radio_transport_udp;
    QRadioButton *radio_transport_bluetooth;
    QRadioButton *radio_transport_lora;
    QPushButton *btn_transport_connect;
    QPushButton *btn_error_lookup;
    QPushButton *btn_cancel;
    QSpacerItem *horizontalSpacer_6;
    QTabWidget *selector_group;
    QWidget *tab_IMG;
    QGridLayout *gridLayout_3;
    QTabWidget *selector_img;
    QWidget *tab_IMG_Upload;
    QGridLayout *gridLayout_4;
    QLabel *label_6;
    QCheckBox *check_IMG_Reset;
    QProgressBar *progress_IMG_Complete;
    QLabel *label_9;
    QLabel *label_4;
    QHBoxLayout *horizontalLayout_5;
    QLineEdit *edit_IMG_Local;
    QToolButton *btn_IMG_Local;
    QLabel *label_41;
    QHBoxLayout *horizontalLayout_4;
    QSpinBox *edit_IMG_Image;
    QRadioButton *radio_IMG_No_Action;
    QRadioButton *radio_IMG_Test;
    QRadioButton *radio_IMG_Confirm;
    QSpacerItem *verticalSpacer_4;
    QWidget *tab_IMG_Images;
    QGridLayout *gridLayout_5;
    QColumnView *colview_IMG_Images;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_5;
    QRadioButton *radio_IMG_Get;
    QRadioButton *radio_IMG_Set;
    QRadioButton *radio_img_images_erase;
    QFrame *line;
    QCheckBox *check_IMG_Confirm;
    QSpacerItem *horizontalSpacer_5;
    QWidget *tab_IMG_Erase;
    QGridLayout *gridLayout_10;
    QLabel *label_14;
    QSpacerItem *horizontalSpacer_9;
    QSpinBox *edit_IMG_Erase_Slot;
    QSpacerItem *verticalSpacer_2;
    QWidget *tab_IMG_Slots;
    QVBoxLayout *verticalLayout_6;
    QTreeWidget *tree_IMG_Slot_Info;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *btn_IMG_Go;
    QSpacerItem *horizontalSpacer_4;
    QLabel *lbl_IMG_Status;
    QWidget *tab_FS;
    QGridLayout *gridLayout_2;
    QLabel *label_28;
    QLabel *lbl_FS_Status;
    QLabel *label_29;
    QLabel *label_2;
    QProgressBar *progress_FS_Complete;
    QToolButton *btn_FS_Local;
    QLabel *label_3;
    QComboBox *combo_FS_type;
    QHBoxLayout *horizontalLayout;
    QRadioButton *radio_FS_Upload;
    QRadioButton *radio_FS_Download;
    QRadioButton *radio_FS_Size;
    QRadioButton *radio_FS_HashChecksum;
    QRadioButton *radio_FS_Hash_Checksum_Types;
    QLabel *label_19;
    QLineEdit *edit_FS_Remote;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *btn_FS_Go;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer_6;
    QLineEdit *edit_FS_Local;
    QLineEdit *edit_FS_Result;
    QLineEdit *edit_FS_Size;
    QWidget *tab_OS;
    QGridLayout *gridLayout_7;
    QHBoxLayout *horizontalLayout_13;
    QSpacerItem *horizontalSpacer_17;
    QPushButton *btn_OS_Go;
    QSpacerItem *horizontalSpacer_18;
    QLabel *lbl_OS_Status;
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
    QVBoxLayout *verticalLayout_4;
    QTableWidget *table_OS_Memory;
    QWidget *tab_OS_Reset;
    QGridLayout *gridLayout_12;
    QCheckBox *check_OS_Force_Reboot;
    QSpacerItem *verticalSpacer_3;
    QWidget *tab_os_datetime;
    QGridLayout *gridLayout_18;
    QSpacerItem *verticalSpacer_8;
    QComboBox *combo_os_datetime_timezone;
    QHBoxLayout *horizontalLayout_19;
    QRadioButton *radio_os_datetime_get;
    QRadioButton *radio_os_datetime_set;
    QSpacerItem *horizontalSpacer_15;
    QLabel *label_13;
    QLabel *label_31;
    QDateTimeEdit *edit_os_datetime_date_time;
    QLabel *label_30;
    QCheckBox *check_os_datetime_use_pc_date_time;
    QWidget *tab_OS_Info;
    QGridLayout *gridLayout_13;
    QLabel *label_17;
    QLineEdit *edit_OS_UName;
    QHBoxLayout *horizontalLayout_10;
    QRadioButton *radio_OS_Buffer_Info;
    QRadioButton *radio_OS_uname;
    QLabel *label_18;
    QPlainTextEdit *edit_OS_Info_Output;
    QWidget *tab_OS_Bootloader;
    QFormLayout *formLayout_2;
    QLabel *label_20;
    QLineEdit *edit_os_bootloader_query;
    QLabel *label_21;
    QLineEdit *edit_os_bootloader_response;
    QWidget *tab_Stats;
    QGridLayout *gridLayout_11;
    QLabel *lbl_STAT_Status;
    QHBoxLayout *horizontalLayout_9;
    QRadioButton *radio_STAT_List;
    QRadioButton *radio_STAT_Fetch;
    QComboBox *combo_STAT_Group;
    QHBoxLayout *horizontalLayout_14;
    QSpacerItem *horizontalSpacer_19;
    QPushButton *btn_STAT_Go;
    QSpacerItem *horizontalSpacer_20;
    QLabel *label_16;
    QLabel *label_15;
    QTableWidget *table_STAT_Values;
    QWidget *tab_Shell;
    QGridLayout *gridLayout_9;
    QLabel *lbl_SHELL_Status;
    AutScrollEdit *edit_SHELL_Output;
    QHBoxLayout *horizontalLayout_8;
    QSpacerItem *horizontalSpacer_7;
    QToolButton *btn_SHELL_Clear;
    QToolButton *btn_SHELL_Copy;
    QSpacerItem *horizontalSpacer_8;
    QHBoxLayout *horizontalLayout_17;
    QCheckBox *check_shell_vt100_decoding;
    QCheckBox *check_shel_unescape_strings;
    QSpacerItem *horizontalSpacer_12;
    QWidget *tab_Settings;
    QGridLayout *gridLayout_15;
    QLineEdit *edit_settings_key;
    QLabel *label_22;
    QLabel *lbl_settings_status;
    QSpacerItem *verticalSpacer_5;
    QLabel *label_26;
    QHBoxLayout *horizontalLayout_16;
    QRadioButton *radio_settings_none;
    QRadioButton *radio_settings_text;
    QRadioButton *radio_settings_decimal;
    QLabel *label_23;
    QLabel *label_24;
    QHBoxLayout *horizontalLayout_15;
    QSpacerItem *horizontalSpacer_10;
    QPushButton *btn_settings_go;
    QSpacerItem *horizontalSpacer_11;
    QLabel *label_25;
    QHBoxLayout *horizontalLayout_12;
    QCheckBox *check_settings_big_endian;
    QCheckBox *check_settings_signed_decimal_value;
    QLabel *label_27;
    QHBoxLayout *horizontalLayout_11;
    QRadioButton *radio_settings_read;
    QRadioButton *radio_settings_write;
    QRadioButton *radio_settings_delete;
    QRadioButton *radio_settings_commit;
    QRadioButton *radio_settings_load;
    QRadioButton *radio_settings_save;
    QLineEdit *edit_settings_value;
    QLineEdit *edit_settings_decoded;
    QFrame *line_2;
    QWidget *tab_zephyr;
    QGridLayout *gridLayout_16;
    QHBoxLayout *horizontalLayout_18;
    QSpacerItem *horizontalSpacer_13;
    QPushButton *btn_zephyr_go;
    QSpacerItem *horizontalSpacer_14;
    QTabWidget *tabWidget_4;
    QWidget *tab_zephyr_storage_erase;
    QGridLayout *gridLayout_17;
    QLabel *label_12;
    QSpacerItem *verticalSpacer_7;
    QLabel *lbl_zephyr_status;
    QWidget *tab_Enum;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_21;
    QRadioButton *radio_Enum_Count;
    QRadioButton *radio_Enum_List;
    QRadioButton *radio_Enum_Single;
    QRadioButton *radio_Enum_Details;
    QHBoxLayout *horizontalLayout_22;
    QLabel *label_32;
    QLineEdit *edit_Enum_Count;
    QHBoxLayout *horizontalLayout_23;
    QLabel *label_33;
    QSpinBox *edit_Enum_Index;
    QFrame *line_3;
    QLabel *label_34;
    QLineEdit *edit_Enum_Group_ID;
    QCheckBox *edit_Enum_Group_Additional;
    QSpacerItem *horizontalSpacer_22;
    QTableWidget *table_Enum_List_Details;
    QLabel *lbl_enum_status;
    QHBoxLayout *horizontalLayout_20;
    QSpacerItem *horizontalSpacer_16;
    QPushButton *btn_enum_go;
    QSpacerItem *horizontalSpacer_21;
    QWidget *tab_custom;
    QVBoxLayout *verticalLayout_5;
    QFormLayout *formLayout_3;
    QLabel *label_38;
    QHBoxLayout *horizontalLayout_26;
    QRadioButton *radio_custom_custom;
    QRadioButton *radio_custom_logging;
    QFrame *line_7;
    QLabel *label_39;
    QRadioButton *radio_custom_json;
    QRadioButton *radio_custom_yaml;
    QRadioButton *radio_custom_cbor;
    QSpacerItem *horizontalSpacer_26;
    QLabel *label_40;
    QHBoxLayout *horizontalLayout_28;
    QRadioButton *radio_custom_read;
    QRadioButton *radio_custom_write;
    QFrame *line_5;
    QLabel *label_411;
    QSpinBox *edit_custom_group;
    QFrame *line_6;
    QLabel *label_42;
    QSpinBox *edit_custom_command;
    QSpacerItem *horizontalSpacer_28;
    QLabel *label_35;
    QPlainTextEdit *edit_custom_send;
    QLabel *label_36;
    QPlainTextEdit *edit_custom_receive;
    QLabel *label_37;
    QHBoxLayout *horizontalLayout_25;
    QSpinBox *edit_custom_indent;
    QFrame *line_4;
    QPushButton *btn_custom_copy_send;
    QPushButton *btn_custom_copy_receive;
    QPushButton *btn_custom_copy_both;
    QPushButton *btn_custom_clear;
    QSpacerItem *horizontalSpacer_25;
    QLabel *lbl_custom_status;
    QHBoxLayout *horizontalLayout_24;
    QSpacerItem *horizontalSpacer_23;
    QPushButton *btn_custom_go;
    QSpacerItem *horizontalSpacer_24;
    QWidget *tab_2;
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
    QButtonGroup *buttonGroup_5;
    QButtonGroup *buttonGroup;
    QButtonGroup *buttonGroup_4;
    QButtonGroup *buttonGroup_6;
    QButtonGroup *buttonGroup_2;
    QButtonGroup *buttonGroup_3;
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
    QStandardItemModel model_image_state;
    error_lookup *error_lookup_form;
    smp_processor *processor;
    smp_group_array smp_groups;
    class smp_uart_auterm *uart_transport;
    uint16_t enum_count;
    QList<uint16_t> enum_groups;
    uint16_t enum_single_id;
    bool enum_single_end;
    QList<enum_details_t> enum_details;
    enum_fields_present_t enum_details_present_fields;
    QList<slot_info_t> img_slot_details;

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    class smp_udp *udp_transport;
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    class smp_bluetooth *bluetooth_transport;
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    class smp_lorawan *lora_transport;
#endif

    QList<image_state_t> images_list;
    QList<hash_checksum_t> supported_hash_checksum_list;
    QVariant bootloader_info_response;
    QByteArray settings_read_response;
    QByteArray fs_hash_checksum_response;
    uint32_t fs_size_response;
#ifndef SKIPPLUGIN_LOGGER
    debug_logger *logger;
#endif
    bool uart_transport_locked;
    QDateTime rtc_time_date_response;
    smp_json *log_json;
};

#endif // PLUGIN_MCUMGR_H
