#ifndef FORM_H
#define FORM_H

#include <QWidget>

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
    void on_radio_transport_uart_toggled(bool checked);
    void on_radio_transport_udp_toggled(bool checked);
    void on_radio_transport_bluetooth_toggled(bool checked);
    void on_radio_OS_Buffer_Info_toggled(bool checked);
    void on_radio_OS_uname_toggled(bool checked);

private:
    Ui::Form *ui;
};

#endif // FORM_H
