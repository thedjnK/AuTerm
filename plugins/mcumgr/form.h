#ifndef FORM_H
#define FORM_H

#include <QWidget>
#include <QTreeWidgetItem>

namespace Ui {
class Form;
}

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = nullptr);
    ~Form();

private slots:
    void on_btn_FS_Local_clicked();
    void on_btn_FS_Go_clicked();
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
    void on_radio_transport_uart_toggled(bool checked);
    void on_radio_transport_udp_toggled(bool checked);
    void on_radio_transport_bluetooth_toggled(bool checked);
    void on_radio_OS_Buffer_Info_toggled(bool checked);
    void on_radio_OS_uname_toggled(bool checked);
    void on_radio_IMG_Get_toggled(bool checked);
    void on_radio_IMG_Set_toggled(bool checked);
    void on_radio_img_settings_read_toggled(bool checked);
    void on_radio_img_settings_write_toggled(bool checked);
    void on_radio_img_settings_delete_toggled(bool checked);
    void on_radio_img_settings_commit_toggled(bool checked);
    void on_radio_img_settings_load_toggled(bool checked);
    void on_radio_img_settings_save_toggled(bool checked);
    void on_btn_os_settings_go_clicked();
    void on_radio_FS_Upload_toggled(bool checked);
    void on_radio_FS_Download_toggled(bool checked);
    void on_radio_FS_Size_toggled(bool checked);
    void on_radio_FS_HashChecksum_toggled(bool checked);
    void on_radio_FS_Hash_Checksum_Types_toggled(bool checked);
    void on_radio_settings_none_toggled(bool checked);
    void on_radio_settings_text_toggled(bool checked);
    void on_radio_settings_decimal_toggled(bool checked);
    void on_check_settings_big_endian_toggled(bool checked);
    void on_check_settings_signed_decimal_value_toggled(bool checked);
    void on_edit_SHELL_Input_returnPressed();
    void on_btn_zephyr_go_clicked();
    void on_check_os_rtc_use_pc_date_time_toggled(bool checked);
    void on_radio_os_rtc_get_toggled(bool checked);
    void on_radio_os_rtc_set_toggled(bool checked);
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
    void on_edit_custom_indent_valueChanged(int arg1);
    void on_btn_custom_go_clicked();
    void on_tree_IMG_Slot_Info_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_btn_error_lookup_clicked();
    void on_btn_cancel_clicked();

private:
    Ui::Form *ui;
};

#endif // FORM_H
