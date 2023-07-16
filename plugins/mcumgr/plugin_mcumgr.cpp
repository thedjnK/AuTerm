/******************************************************************************
** Copyright (C) 2021-2023 Jamie M.
**
** Project: AuTerm
**
** Module:  plugin_mcumgr.cpp
**
** Notes:   Conversion of a conversion of an unfinished multi transport
**          testing application
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
#include "plugin_mcumgr.h"
#include <QDebug>
#include <QFileDialog>
#include "smp_uart.h"

#include <QStandardItemModel>

QMainWindow *parent_window;
smp_uart *uart;
QStandardItemModel model_image_state;

//0x08 = new version, 0x00 = old
#define setup_smp_message(message, stream, write, group, id) \
message.append((char)(0x00 | (write == true ? 0x02 : 0x00)));  /* Read | Write (0x00 | 0x02) */ \
    message.append((char)0x00);  /* Flags */ \
    message.append((char)0x00);  /* Length A */ \
    message.append((char)0x05);  /* Length B */ \
    message.append((char)(group >> 8));  /* Group A */ \
    message.append((char)group);  /* Group B */ \
    message.append((char)0x01);  /* Sequence */ \
    message.append((char)id);   /* Message ID */ \
    stream.startMap()

#define finish_smp_message(message, stream, data) \
    stream.endMap(); \
    message[2] = (uint8_t)(smp_data.length() >> 8); \
    message[3] = (uint8_t)smp_data.length(); \
    message.append(data)

void plugin_mcumgr::setup(QMainWindow *main_window)
{
	uart = new smp_uart(NULL);

	file_upload_in_progress = false;
	file_list_in_progress = false;
	file_upload_area = 0;

	parent_window = main_window;
    QTabWidget *tabWidget_orig = main_window->findChild<QTabWidget *>("selector_Tab");
//    QPushButton *other = main_window->findChild<QPushButton *>("btn_TermClose");

//    gridLayout = new QGridLayout();
//    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
//    tabWidget = new QTabWidget(tabWidget_orig);
//    tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
    tab = new QWidget(tabWidget_orig);
    tab->setObjectName("tab");
    label = new QLabel(tab);
    label->setObjectName("label");
    label->setGeometry(QRect(10, 20, 54, 17));
    tabWidget_2 = new QTabWidget(tab);
    tabWidget_2->setObjectName("tabWidget_2");
    tabWidget_2->setGeometry(QRect(10, 40, 379, 321));
    tabWidget_2->setTabPosition(QTabWidget::West);
    tab_2 = new QWidget();
    tab_2->setObjectName("tab_2");
    gridLayout_2 = new QGridLayout(tab_2);
    gridLayout_2->setSpacing(2);
    gridLayout_2->setObjectName("gridLayout_2");
    gridLayout_2->setContentsMargins(6, 6, 6, 6);
    lbl_FS_Status = new QLabel(tab_2);
    lbl_FS_Status->setObjectName("lbl_FS_Status");

    gridLayout_2->addWidget(lbl_FS_Status, 4, 0, 1, 2);

    label_2 = new QLabel(tab_2);
    label_2->setObjectName("label_2");

    gridLayout_2->addWidget(label_2, 0, 0, 1, 1);

    progress_FS_Complete = new QProgressBar(tab_2);
    progress_FS_Complete->setObjectName("progress_FS_Complete");
    progress_FS_Complete->setValue(24);

    gridLayout_2->addWidget(progress_FS_Complete, 3, 0, 1, 3);

    edit_FS_Log = new QPlainTextEdit(tab_2);
    edit_FS_Log->setObjectName("edit_FS_Log");
    edit_FS_Log->setUndoRedoEnabled(false);
    edit_FS_Log->setReadOnly(true);

    gridLayout_2->addWidget(edit_FS_Log, 7, 0, 1, 3);

    edit_FS_Remote = new QLineEdit(tab_2);
    edit_FS_Remote->setObjectName("edit_FS_Remote");

    gridLayout_2->addWidget(edit_FS_Remote, 1, 1, 1, 2);

    btn_FS_Local = new QToolButton(tab_2);
    btn_FS_Local->setObjectName("btn_FS_Local");

    gridLayout_2->addWidget(btn_FS_Local, 0, 2, 1, 1);

    edit_FS_Local = new QLineEdit(tab_2);
    edit_FS_Local->setObjectName("edit_FS_Local");

    gridLayout_2->addWidget(edit_FS_Local, 0, 1, 1, 1);

    label_3 = new QLabel(tab_2);
    label_3->setObjectName("label_3");

    gridLayout_2->addWidget(label_3, 1, 0, 1, 1);

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName("horizontalLayout");
    radio_FS_Upload = new QRadioButton(tab_2);
    radio_FS_Upload->setObjectName("radio_FS_Upload");
    radio_FS_Upload->setChecked(true);

    horizontalLayout->addWidget(radio_FS_Upload);

    radio_FS_Download = new QRadioButton(tab_2);
    radio_FS_Download->setObjectName("radio_FS_Download");

    horizontalLayout->addWidget(radio_FS_Download);

    radio_FS_Size = new QRadioButton(tab_2);
    radio_FS_Size->setObjectName("radio_FS_Size");

    horizontalLayout->addWidget(radio_FS_Size);

    radio_FS_HashChecksum = new QRadioButton(tab_2);
    radio_FS_HashChecksum->setObjectName("radio_FS_HashChecksum");

    horizontalLayout->addWidget(radio_FS_HashChecksum);


    gridLayout_2->addLayout(horizontalLayout, 2, 0, 1, 3);

    horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setObjectName("horizontalLayout_2");
    horizontalLayout_2->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_2->addItem(horizontalSpacer);

    btn_FS_Go = new QPushButton(tab_2);
    btn_FS_Go->setObjectName("btn_FS_Go");

    horizontalLayout_2->addWidget(btn_FS_Go);

    horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_2->addItem(horizontalSpacer_2);


    gridLayout_2->addLayout(horizontalLayout_2, 5, 0, 1, 3);

    tabWidget_2->addTab(tab_2, QString());
    tab_3 = new QWidget();
    tab_3->setObjectName("tab_3");
    gridLayout_3 = new QGridLayout(tab_3);
    gridLayout_3->setSpacing(2);
    gridLayout_3->setObjectName("gridLayout_3");
    gridLayout_3->setContentsMargins(6, 6, 6, 6);
    edit_IMG_Log = new QPlainTextEdit(tab_3);
    edit_IMG_Log->setObjectName("edit_IMG_Log");
    edit_IMG_Log->setUndoRedoEnabled(false);
    edit_IMG_Log->setReadOnly(true);

    gridLayout_3->addWidget(edit_IMG_Log, 7, 0, 1, 3);

    horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setObjectName("horizontalLayout_3");
    horizontalLayout_3->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_3->addItem(horizontalSpacer_3);

    btn_IMG_Go = new QPushButton(tab_3);
    btn_IMG_Go->setObjectName("btn_IMG_Go");

    horizontalLayout_3->addWidget(btn_IMG_Go);

    horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_3->addItem(horizontalSpacer_4);


    gridLayout_3->addLayout(horizontalLayout_3, 5, 0, 1, 3);

    lbl_IMG_Status = new QLabel(tab_3);
    lbl_IMG_Status->setObjectName("lbl_IMG_Status");

    gridLayout_3->addWidget(lbl_IMG_Status, 3, 0, 1, 2);

    tabWidget_3 = new QTabWidget(tab_3);
    tabWidget_3->setObjectName("tabWidget_3");
    tab_10 = new QWidget();
    tab_10->setObjectName("tab_10");
    gridLayout_4 = new QGridLayout(tab_10);
    gridLayout_4->setSpacing(2);
    gridLayout_4->setObjectName("gridLayout_4");
    gridLayout_4->setContentsMargins(6, 6, 6, 6);
    horizontalLayout_4 = new QHBoxLayout();
    horizontalLayout_4->setSpacing(2);
    horizontalLayout_4->setObjectName("horizontalLayout_4");
    edit_IMG_Image = new QSpinBox(tab_10);
    edit_IMG_Image->setObjectName("edit_IMG_Image");
    edit_IMG_Image->setMaximumSize(QSize(60, 16777215));

    horizontalLayout_4->addWidget(edit_IMG_Image);

    radio_IMG_No_Action = new QRadioButton(tab_10);
    radio_IMG_No_Action->setObjectName("radio_IMG_No_Action");

    horizontalLayout_4->addWidget(radio_IMG_No_Action);

    radio_IMG_Test = new QRadioButton(tab_10);
    radio_IMG_Test->setObjectName("radio_IMG_Test");
    radio_IMG_Test->setChecked(true);

    horizontalLayout_4->addWidget(radio_IMG_Test);

    radio_IMG_Confirm = new QRadioButton(tab_10);
    radio_IMG_Confirm->setObjectName("radio_IMG_Confirm");

    horizontalLayout_4->addWidget(radio_IMG_Confirm);


    gridLayout_4->addLayout(horizontalLayout_4, 1, 1, 1, 1);

    horizontalLayout_5 = new QHBoxLayout();
    horizontalLayout_5->setSpacing(2);
    horizontalLayout_5->setObjectName("horizontalLayout_5");
    edit_IMG_Local = new QLineEdit(tab_10);
    edit_IMG_Local->setObjectName("edit_IMG_Local");

    horizontalLayout_5->addWidget(edit_IMG_Local);

    btn_IMG_Local = new QToolButton(tab_10);
    btn_IMG_Local->setObjectName("btn_IMG_Local");

    horizontalLayout_5->addWidget(btn_IMG_Local);


    gridLayout_4->addLayout(horizontalLayout_5, 0, 1, 1, 1);

    label_4 = new QLabel(tab_10);
    label_4->setObjectName("label_4");

    gridLayout_4->addWidget(label_4, 1, 0, 1, 1);

    label_41 = new QLabel(tab_10);
    label_41->setObjectName("label_41");

    gridLayout_4->addWidget(label_41, 0, 0, 1, 1);

    progress_IMG_Complete = new QProgressBar(tab_10);
    progress_IMG_Complete->setObjectName("progress_IMG_Complete");
    progress_IMG_Complete->setValue(24);

    gridLayout_4->addWidget(progress_IMG_Complete, 2, 1, 1, 1);

    label_6 = new QLabel(tab_10);
    label_6->setObjectName("label_6");

    gridLayout_4->addWidget(label_6, 2, 0, 1, 1);

    tabWidget_3->addTab(tab_10, QString());
    tab_9 = new QWidget();
    tab_9->setObjectName("tab_9");
    gridLayout_5 = new QGridLayout(tab_9);
    gridLayout_5->setSpacing(2);
    gridLayout_5->setObjectName("gridLayout_5");
    gridLayout_5->setContentsMargins(6, 6, 6, 6);
    colview_IMG_Images = new QColumnView(tab_9);
    colview_IMG_Images->setObjectName("colview_IMG_Images");

    gridLayout_5->addWidget(colview_IMG_Images, 0, 0, 1, 1);

    horizontalLayout_6 = new QHBoxLayout();
    horizontalLayout_6->setObjectName("horizontalLayout_6");
    label_5 = new QLabel(tab_9);
    label_5->setObjectName("label_5");

    horizontalLayout_6->addWidget(label_5);

    radio_IMG_Get = new QRadioButton(tab_9);
    radio_IMG_Get->setObjectName("radio_IMG_Get");
    radio_IMG_Get->setChecked(true);

    horizontalLayout_6->addWidget(radio_IMG_Get);

    radio_ING_Set = new QRadioButton(tab_9);
    radio_ING_Set->setObjectName("radio_ING_Set");

    horizontalLayout_6->addWidget(radio_ING_Set);

    horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_6->addItem(horizontalSpacer_5);


    gridLayout_5->addLayout(horizontalLayout_6, 1, 0, 1, 1);

    tabWidget_3->addTab(tab_9, QString());

    gridLayout_3->addWidget(tabWidget_3, 0, 0, 1, 1);

    tabWidget_2->addTab(tab_3, QString());
    tab_4 = new QWidget();
    tab_4->setObjectName("tab_4");
    tabWidget_2->addTab(tab_4, QString());
    tab_5 = new QWidget();
    tab_5->setObjectName("tab_5");
    tabWidget_2->addTab(tab_5, QString());
    tab_6 = new QWidget();
    tab_6->setObjectName("tab_6");
    tabWidget_2->addTab(tab_6, QString());
    edit_MTU = new QSpinBox(tab);
    edit_MTU->setObjectName("edit_MTU");
    edit_MTU->setGeometry(QRect(50, 10, 61, 26));
    edit_MTU->setMinimum(32);
    edit_MTU->setMaximum(2048);
    edit_MTU->setValue(128);
    verticalLayoutWidget = new QWidget();
    verticalLayoutWidget->setObjectName("verticalLayoutWidget");
    verticalLayoutWidget->setGeometry(QRect(6, 6, 229, 182));
    verticalLayout = new QVBoxLayout(verticalLayoutWidget);
    verticalLayout->setSpacing(2);
    verticalLayout->setObjectName("verticalLayout");
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    formLayout = new QFormLayout();
    formLayout->setObjectName("formLayout");
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setHorizontalSpacing(2);
    formLayout->setVerticalSpacing(2);
    label_7 = new QLabel(verticalLayoutWidget);
    label_7->setObjectName("label_7");

    formLayout->setWidget(0, QFormLayout::LabelRole, label_7);

    edit_IMG_Preview_Hash = new QLineEdit(verticalLayoutWidget);
    edit_IMG_Preview_Hash->setObjectName("edit_IMG_Preview_Hash");

    formLayout->setWidget(0, QFormLayout::FieldRole, edit_IMG_Preview_Hash);

    label_8 = new QLabel(verticalLayoutWidget);
    label_8->setObjectName("label_8");

    formLayout->setWidget(1, QFormLayout::LabelRole, label_8);

    edit_IMG_Preview_Version = new QLineEdit(verticalLayoutWidget);
    edit_IMG_Preview_Version->setObjectName("edit_IMG_Preview_Version");

    formLayout->setWidget(1, QFormLayout::FieldRole, edit_IMG_Preview_Version);


    verticalLayout->addLayout(formLayout);

    gridLayout_6 = new QGridLayout();
    gridLayout_6->setSpacing(2);
    gridLayout_6->setObjectName("gridLayout_6");
    check_IMG_Preview_Confirmed = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Confirmed->setObjectName("check_IMG_Preview_Confirmed");

    gridLayout_6->addWidget(check_IMG_Preview_Confirmed, 1, 0, 1, 1);

    check_IMG_Preview_Active = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Active->setObjectName("check_IMG_Preview_Active");
    check_IMG_Preview_Active->setEnabled(true);
    check_IMG_Preview_Active->setCheckable(true);

    gridLayout_6->addWidget(check_IMG_Preview_Active, 0, 0, 1, 1);

    check_IMG_Preview_Pending = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Pending->setObjectName("check_IMG_Preview_Pending");

    gridLayout_6->addWidget(check_IMG_Preview_Pending, 1, 1, 1, 1);

    check_IMG_Preview_Bootable = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Bootable->setObjectName("check_IMG_Preview_Bootable");

    gridLayout_6->addWidget(check_IMG_Preview_Bootable, 0, 1, 1, 1);

    check_IMG_Preview_Permanent = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Permanent->setObjectName("check_IMG_Preview_Permanent");

    gridLayout_6->addWidget(check_IMG_Preview_Permanent, 2, 0, 1, 1);


    verticalLayout->addLayout(gridLayout_6);

    btn_IMG_Preview_Copy = new QPushButton(verticalLayoutWidget);
    btn_IMG_Preview_Copy->setObjectName("btn_IMG_Preview_Copy");

    verticalLayout->addWidget(btn_IMG_Preview_Copy);

    verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout->addItem(verticalSpacer);


//    tabWidget->addTab(tab, QString());

//    gridLayout->addWidget(tabWidget, 0, 0, 1, 1);


//    retranslateUi(Form);

//    tabWidget->setCurrentIndex(0);
    tabWidget_2->setCurrentIndex(1);
    tabWidget_3->setCurrentIndex(1);


//    QMetaObject::connectSlotsByName(Form);

    //retranslate code
//    Form->setWindowTitle(QCoreApplication::translate("Form", "Form", nullptr));
    label->setText(QCoreApplication::translate("Form", "MTU:", nullptr));
    lbl_FS_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    label_2->setText(QCoreApplication::translate("Form", "Local file:", nullptr));
    btn_FS_Local->setText(QCoreApplication::translate("Form", "...", nullptr));
    label_3->setText(QCoreApplication::translate("Form", "Device file:", nullptr));
    radio_FS_Upload->setText(QCoreApplication::translate("Form", "Upload", nullptr));
    radio_FS_Download->setText(QCoreApplication::translate("Form", "Download", nullptr));
    radio_FS_Size->setText(QCoreApplication::translate("Form", "Size", nullptr));
    radio_FS_HashChecksum->setText(QCoreApplication::translate("Form", "Hash/checksum", nullptr));
    btn_FS_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_2), QCoreApplication::translate("Form", "FS", nullptr));
    btn_IMG_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    lbl_IMG_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    radio_IMG_No_Action->setText(QCoreApplication::translate("Form", "No action", nullptr));
    radio_IMG_Test->setText(QCoreApplication::translate("Form", "Test", nullptr));
    radio_IMG_Confirm->setText(QCoreApplication::translate("Form", "Confirm", nullptr));
    btn_IMG_Local->setText(QCoreApplication::translate("Form", "...", nullptr));
    label_4->setText(QCoreApplication::translate("Form", "Image:", nullptr));
    label_41->setText(QCoreApplication::translate("Form", "File:", nullptr));
    label_6->setText(QCoreApplication::translate("Form", "Progress:", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_10), QCoreApplication::translate("Form", "Upload", nullptr));
    label_5->setText(QCoreApplication::translate("Form", "State:", nullptr));
    radio_IMG_Get->setText(QCoreApplication::translate("Form", "Get", nullptr));
    radio_ING_Set->setText(QCoreApplication::translate("Form", "Set", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_9), QCoreApplication::translate("Form", "Images", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_3), QCoreApplication::translate("Form", "Img", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_4), QCoreApplication::translate("Form", "OS", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_5), QCoreApplication::translate("Form", "Stat", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_6), QCoreApplication::translate("Form", "Shell", nullptr));
    label_7->setText(QCoreApplication::translate("Form", "Hash:", nullptr));
    label_8->setText(QCoreApplication::translate("Form", "Version:", nullptr));
    check_IMG_Preview_Confirmed->setText(QCoreApplication::translate("Form", "Confirmed", nullptr));
    check_IMG_Preview_Active->setText(QCoreApplication::translate("Form", "Active", nullptr));
    check_IMG_Preview_Pending->setText(QCoreApplication::translate("Form", "Pending", nullptr));
    check_IMG_Preview_Bootable->setText(QCoreApplication::translate("Form", "Bootable", nullptr));
    check_IMG_Preview_Permanent->setText(QCoreApplication::translate("Form", "Permanent", nullptr));
    btn_IMG_Preview_Copy->setText(QCoreApplication::translate("Form", "Copy", nullptr));
    //tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("Form", "MCUmgr", nullptr));

    //Add code
    tabWidget_orig->addTab(tab, QString("MCUmgr"));

    //QMetaObject::connectSlotsByName(Form);

//Signals
    connect(uart, SIGNAL(serial_write(QByteArray*)), main_window, SLOT(plugin_serial_transmit(QByteArray*)));
    connect(uart, SIGNAL(receive_waiting(QByteArray)), this, SLOT(receive_waiting(QByteArray)));
    connect(btn_IMG_Local, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Local_clicked()));
    connect(btn_IMG_Go, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Go_clicked()));

    colview_IMG_Images->setModel(&model_image_state);
    colview_IMG_Images->setPreviewWidget(verticalLayoutWidget);

    check_IMG_Preview_Confirmed->setChecked(true);
    check_IMG_Preview_Confirmed->installEventFilter(this);
    check_IMG_Preview_Active->installEventFilter(this);
    check_IMG_Preview_Pending->installEventFilter(this);
    check_IMG_Preview_Bootable->installEventFilter(this);
    check_IMG_Preview_Permanent->installEventFilter(this);
}

bool plugin_mcumgr::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress ||
	event->type() == QEvent::MouseButtonRelease ||
	event->type() == QEvent::MouseButtonDblClick ||
	event->type() == QEvent::KeyPress ||
	event->type() == QEvent::KeyRelease ||
	event->type() == QEvent::InputMethod ||
	event->type() == QEvent::ActivationChange ||
	event->type() == QEvent::ModifiedChange)
    {
	    return true;
    }

    return false;
}

const QString plugin_mcumgr::plugin_about()
{
    return "AuTerminal MCUmgr plugin\r\nCopyright 2021-2023 Jamie M.\r\n\r\nCan be used to communicate with Zephyr devices with the serial MCUmgr transport enabled.\r\n\r\nUNFINISHED INITIAL TEST USE ONLY, NOT REPRESENTATIVE OF FINAL PRODUCT.\r\n\r\nBuilt using Qt " QT_VERSION_STR;
}

bool plugin_mcumgr::plugin_configuration()
{
//    emit show_message_box("CONFIG BUTTON was clicked");
    return false;
}

QWidget *plugin_mcumgr::GetWidget()
{
    return 0;
}

void plugin_mcumgr::serial_error(QSerialPort::SerialPortError serial_error)
{
    qDebug() << "error: " << serial_error;
}

void plugin_mcumgr::serial_receive(QByteArray *data)
{
    uart->serial_read(data);
}

void plugin_mcumgr::serial_bytes_written(qint64 bytes)
{
    qDebug() << "written: " << bytes;
}

void plugin_mcumgr::serial_about_to_close()
{
    qDebug() << "about to close";
}

void plugin_mcumgr::on_btn_IMG_Local_clicked()
{
    QString strFilename = QFileDialog::getOpenFileName(parent_window, tr("Open firmware file"), edit_IMG_Local->text(), tr("Binary Files (*.bin);;All Files (*.*)"));

    if (!strFilename.isEmpty())
    {
	    edit_IMG_Local->setText(strFilename);
    }
}

void plugin_mcumgr::on_btn_IMG_Go_clicked()
{
    if (tabWidget_3->currentIndex() == 0)
    {
	    //Upload
	    emit plugin_set_status(true, false);

	    lbl_IMG_Status->setText("Checking...");

	    QFile file(edit_IMG_Local->text());

	    if (!file.open(QFile::ReadOnly))
	    {
		    return;
	    }

	    file_upload_data.clear();
	    file_upload_data.append(file.readAll());

	    file.close();

	    if (extract_hash(&file_upload_data) == false)
	    {
		    lbl_IMG_Status->setText("Hash was not found");
		    file_upload_data.clear();
		    return;
	    }

		   //Send start
	    file_upload_area = 0;

		   //Generate image data
	    QByteArray session_hash = QCryptographicHash::hash(file_upload_data, QCryptographicHash::Sha256);

	    upload_tmr.start();

//	    qDebug() << "hash: " << session_hash.toHex();

	    QByteArray message;
	    QByteArray smp_data;
	    QCborStreamWriter smp_stream(&smp_data);

	    setup_smp_message(message, smp_stream, true, 0x01, 0x01);

	    smp_stream.append("image");
	    smp_stream.append(edit_IMG_Image->value());
	    smp_stream.append("len");
	    smp_stream.append(file_upload_data.length());
	    smp_stream.append("off");
	    smp_stream.append(file_upload_area);
	    smp_stream.append("sha");
	    smp_stream.append(session_hash);
	    smp_stream.append("data");
	    smp_stream.append(file_upload_data.left(edit_MTU->text().toUInt()));
//	    qDebug() << "bytes: " << edit_MTU->text().toUInt();

	    finish_smp_message(message, smp_stream, smp_data);

	    file_upload_in_progress = true;

//	    qDebug() << "len: " << message.length();

	    uart->send(&message);

	    progress_IMG_Complete->setValue(0);
	    lbl_IMG_Status->setText("Uploading...");
    }
    else if (tabWidget_3->currentIndex() == 1)
    {
	    //Image list
	    emit plugin_set_status(true, false);

	    QByteArray message;
	    QByteArray smp_data;
	    QCborStreamWriter smp_stream(&smp_data);

	    setup_smp_message(message, smp_stream, false, 0x01, 0x00);
	    finish_smp_message(message, smp_stream, smp_data);

	    file_list_in_progress = true;

//	    qDebug() << "len: " << message.length();

	    uart->send(&message);

	    progress_IMG_Complete->setValue(0);
	    lbl_IMG_Status->setText("Querying...");
    }
}

bool plugin_mcumgr::handleStream_upload(QCborStreamReader &reader, int32_t *new_rc, int64_t *new_off)
{
//    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    int32_t rc = -1;
    int64_t off = -1;

    while (!reader.lastError() && reader.hasNext())
    {
	    bool keyset = false;
//	    qDebug() << "Key: " << key;
//	    qDebug() << "Type: " << reader.type();
	    switch (reader.type())
	    {
	    case QCborStreamReader::UnsignedInteger:
	    case QCborStreamReader::NegativeInteger:
	    case QCborStreamReader::SimpleType:
	    case QCborStreamReader::Float16:
	    case QCborStreamReader::Float:
	    case QCborStreamReader::Double:
	    {
		    //	handleFixedWidth(reader);
		    if (key == "rc")
		    {
//			    qDebug() << "found rc";
			    rc = reader.toInteger();
		    }
		    else if (key == "off")
		    {
//			    qDebug() << "found off";
			    off = reader.toInteger();
		    }

		    reader.next();
		    break;
	    }
	    case QCborStreamReader::ByteArray:
	    {
		    auto r = reader.readByteArray();
		    while (r.status == QCborStreamReader::Ok)
		    {
			    r = reader.readByteArray();
		    }
	    }
	    break;
	    case QCborStreamReader::String:
	    {
		    QString data;
		    auto r = reader.readString();
		    while (r.status == QCborStreamReader::Ok)
		    {
			    data.append(r.data);
			    r = reader.readString();
		    }

		    if (r.status == QCborStreamReader::Error)
		    {
			    data.clear();
			    qDebug("Error decoding string");
		    }
		    else
		    {
			    if (key.isEmpty())
			    {
				    key = data;
				    keyset = true;
			    }
		    }
		    break;
	    }
	    case QCborStreamReader::Array:
	    case QCborStreamReader::Map:
		    reader.enterContainer();
		    while (reader.lastError() == QCborError::NoError && reader.hasNext())
		    {
			    handleStream_upload(reader, new_rc, new_off);
		    }
		    if (reader.lastError() == QCborError::NoError)
		    {
			    reader.leaveContainer();
		    }
		    break;
	    }

	    if (keyset == false && !key.isEmpty())
	    {
		    key = "";
	    }
    }

    if (new_rc != NULL && rc != -1)
    {
	    *new_rc = rc;
    }

    if (new_off != NULL && off != -1)
    {
	    *new_off = off;
    }

    return true;
}

struct blah {
uint32_t image;
uint32_t slot;
QByteArray version;
QByteArray hash;
bool bootable;
bool pending;
bool confirmed;
bool active;
bool permanent;
bool splitstatus;
};

QList<blah> blaharray;
blah thisblah;

bool plugin_mcumgr::handleStream_state(QCborStreamReader &reader, int32_t *new_rc, QString array_name)
{
QString array_name_dupe = array_name;
    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    int32_t rc = -1;
    int64_t off = -1;

    thisblah.image = 0;
    thisblah.slot = 0;
    thisblah.version.clear();
    thisblah.hash.clear();
    thisblah.bootable = false;
    thisblah.pending = false;
    thisblah.confirmed = false;
    thisblah.active = false;
    thisblah.permanent = false;
    thisblah.splitstatus = false;
    uint8_t items = 0;

    while (!reader.lastError() && reader.hasNext())
    {
	    bool keyset = false;
//	    qDebug() << "Key: " << key;
//	    qDebug() << "Type: " << reader.type();
	    switch (reader.type())
	    {
	    case QCborStreamReader::SimpleType:
	    {
		    bool *index = NULL;
		    if (key == "bootable")
		    {
			    index = &thisblah.bootable;
		    }
		    else if (key == "pending")
		    {
			    index = &thisblah.pending;
		    }
		    else if (key == "confirmed")
		    {
			    index = &thisblah.confirmed;
		    }
		    else if (key == "active")
		    {
			    index = &thisblah.active;
		    }
		    else if (key == "permanent")
		    {
			    index = &thisblah.permanent;
		    }
		    else if (key == "splitStatus")
		    {
			    index = &thisblah.splitstatus;
		    }

		    if (index != NULL)
		    {
			    *index = reader.toBool();
		    }

		    reader.next();
		    break;
	    }

	    case QCborStreamReader::UnsignedInteger:
	    case QCborStreamReader::NegativeInteger:
	    case QCborStreamReader::Float16:
	    case QCborStreamReader::Float:
	    case QCborStreamReader::Double:
	    {
		    //	handleFixedWidth(reader);
		    if (key == "rc")
		    {
//			    qDebug() << "found rc";
			    rc = reader.toInteger();
		    }
		    else if (key == "image")
		    {
			    thisblah.image = reader.toUnsignedInteger();
		    }
		    else if (key == "slot")
		    {
			    thisblah.slot = reader.toUnsignedInteger();
		    }

		    reader.next();
		    break;
	    }
	    case QCborStreamReader::ByteArray:
	    {
		    QByteArray data;
		    auto r = reader.readByteArray();
		    while (r.status == QCborStreamReader::Ok)
		    {
			    data.append(r.data);
			    r = reader.readByteArray();
		    }

		    if (key == "hash")
		    {
			    thisblah.hash = data;
			    items |= 0x01;
		    }
	    }
	    break;
	    case QCborStreamReader::String:
	    {
		    QString data;
		    auto r = reader.readString();
		    while (r.status == QCborStreamReader::Ok)
		    {
			    data.append(r.data);
			    r = reader.readString();
		    }

		    if (r.status == QCborStreamReader::Error)
		    {
			    data.clear();
			    qDebug("Error decoding string");
		    }
		    else
		    {
			    if (key.isEmpty())
			    {
				    key = data;
				    keyset = true;
			    }
			    else if (key == "version")
			    {
				    thisblah.version = data.toUtf8();
				    items |= 0x02;
			    }
		    }
		    break;
	    }
	    case QCborStreamReader::Array:
	    case QCborStreamReader::Map:

		    if (reader.type() == QCborStreamReader::Array)
		    {
			    array_name_dupe = key;
		    }
		    reader.enterContainer();
		    while (reader.lastError() == QCborError::NoError && reader.hasNext())
		    {
			    qDebug() << "container/map";
			    handleStream_state(reader, new_rc, array_name_dupe);
//			    if (key == "images")
//			    {
//				    blaharray.append(thisblah);
//			    }
		    }
		    if (reader.lastError() == QCborError::NoError)
		    {
			    qDebug() << "leave";
			    reader.leaveContainer();

			    if (array_name == "images")
			    {
				    blaharray.append(thisblah);
				    QStandardItem *group = new QStandardItem(QString("Image %1").arg(thisblah.image));
				    QStandardItem *child = new QStandardItem(QString("Slot %1").arg(thisblah.slot));
				    group->appendRow(child);
				    model_image_state.appendRow(group);
				    model_image_state.setColumnCount(1);
				    model_image_state.setRowCount(1);
			    }
		    }
		    break;
	    }

	    if (keyset == false && !key.isEmpty())
	    {
		    key = "";
	    }
    }

    if (new_rc != NULL && rc != -1)
    {
	    *new_rc = rc;
    }

    return true;
}

bool plugin_mcumgr::extract_hash(QByteArray *file_data)
{
    bool found = false;

    int32_t pos = file_data->length() - 4;
    int16_t length;
    while (pos >= 0)
    {
	    if (file_data->mid(pos, 2) == image_tlv_magic)
	    {
		    length = file_data->at(pos + 2);
		    length |= ((uint16_t)file_data->at(pos + 3)) << 8;

		    if ((pos + length) == file_data->length())
		    {
			    found = true;
			    break;
		    }
	    }

	    --pos;
    }

    if (found == true)
    {
	    int32_t new_pos = pos + 4;
	    while (new_pos < file_data->length())
	    {
		    uint8_t type = file_data->at(new_pos);
		    int16_t local_length = file_data->at(new_pos + 2);
		    local_length |= ((uint16_t)file_data->at(new_pos + 3)) << 8;

//		    qDebug() << "Type " << type << ", length " << local_length;

		    if (type == 0x10 && local_length == 32)
		    {
			    //We have the hash we wanted
			    upload_hash = file_data->mid((new_pos + 4), local_length);
			    //todo: check if another hash is present?
			    return true;
		    }

		    new_pos += local_length + 4;
	    }
    }

    return false;
}

void plugin_mcumgr::file_upload(QByteArray *message)
{
    message->remove(0, 8);
    QCborStreamReader cbor_reader(*message);
    int32_t rc = -1;
    int64_t off = -1;
    bool good = handleStream_upload(cbor_reader, &rc, &off);

//    qDebug() << "rc = " << rc << ", off = " << off;

    if (off != -1 && rc != 9)
    {
	    file_upload_area = off;
    }
//    qDebug() << "good is " << good;

    if (file_upload_area != 0)
    {
	    progress_IMG_Complete->setValue(file_upload_area * 100 / file_upload_data.length());
    }

    if (good == true)
    {
	    //Upload next chunk
	    if (file_upload_area >= file_upload_data.length())
	    {
		    float blah = file_upload_data.length();
		    uint8_t prefix = 0;
		    while (blah >= 1024)
		    {
			    blah /= 1024;
			    ++prefix;
		    }
		    QString bob;
		    if (prefix == 0)
		    {
			    bob = "B";
		    }
		    else if (prefix == 1)
		    {
			    bob = "KiB";
		    }
		    else if (prefix == 2)
		    {
			    bob = "MiB";
		    }
		    else if (prefix == 3)
		    {
			    bob = "GiB";
		    }

		    blah = file_upload_data.length() / (float)(upload_tmr.elapsed() / 1000);
		    prefix = 0;
		    while (blah >= 1024)
		    {
			    blah /= 1024;
			    ++prefix;
		    }

		    if (prefix == 0)
		    {
			    bob = "B";
		    }
		    else if (prefix == 1)
		    {
			    bob = "KiB";
		    }
		    else if (prefix == 2)
		    {
			    bob = "MiB";
		    }
		    else if (prefix == 3)
		    {
			    bob = "GiB";
		    }
		    edit_IMG_Log->appendPlainText(QString("~").append(QString::number(blah)).append(bob).append("ps throughput"));

		    file_upload_data.clear();

		    if (radio_IMG_Test->isChecked() || radio_IMG_Confirm->isChecked())
		    {
			    QByteArray message;
			    QByteArray smp_data;
			    QCborStreamWriter smp_stream(&smp_data);

			    setup_smp_message(message, smp_stream, true, 0x01, 0x00);

			    smp_stream.append("hash");
			    smp_stream.append(upload_hash);
			    if (radio_IMG_Confirm->isChecked())
			    {
				smp_stream.append("confirm");
				smp_stream.append(false);
			    }

			    finish_smp_message(message, smp_stream, smp_data);
//			    qDebug() << message;
//			    qDebug() << "hash is " << upload_hash;

			    uart->send(&message);
			    lbl_IMG_Status->setText(QString("Marking image ").append(radio_IMG_Test->isChecked() ? "for test." : "as confirmed."));
		    }
		    else
		    {
			    file_upload_in_progress = false;
			    upload_tmr.invalidate();
			    upload_hash.clear();
			    file_upload_area = 0;
			    emit plugin_set_status(false, false);
			    lbl_IMG_Status->setText("Finished.");
		    }

		    return;
	    }

	    QByteArray message;
	    QByteArray smp_data;
	    QCborStreamWriter smp_stream(&smp_data);

	    setup_smp_message(message, smp_stream, true, 0x01, 0x01);

	    smp_stream.append("off");
	    smp_stream.append(file_upload_area);
	    smp_stream.append("data");
	    smp_stream.append(file_upload_data.mid(file_upload_area, edit_MTU->text().toUInt()));

//	    qDebug() << "off: " << file_upload_area << ", left: " << file_upload_data.length();

	    finish_smp_message(message, smp_stream, smp_data);

//	    qDebug() << "len: " << smp_data.length();

	    uart->send(&message);
    }
    else
    {
	    file_upload_in_progress = false;
    }
}

void plugin_mcumgr::receive_waiting(QByteArray message)
{
	uint16_t group = message.at(4);
    group <<= 8;
    group |= message.at(5);
    uint8_t command = (uint8_t)message.at(7);
    if (file_upload_in_progress == true && group == 1)
    {
	    if (command == 0x00)
	    {
		    //Response to set image state
		    int32_t rc = -1;
		    message.remove(0, 8);
		    QCborStreamReader cbor_reader(message);
		    bool good = handleStream_state(cbor_reader, &rc, "");
//		    qDebug() << "Got " << good << ", " << rc;

//		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));
		    lbl_IMG_Status->setText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));

		    file_upload_in_progress = false;
		    upload_tmr.invalidate();
		    upload_hash.clear();
		    file_upload_area = 0;
		    emit plugin_set_status(false, false);
	    }
	    else if (command == 0x01)
	    {
		    file_upload(&message);
	    }
	    else
	    {
		    qDebug() << "Unexpected command ID: " << command;
	    }
    }
    else if (file_list_in_progress == true && group == 1)
    {
	    if (command == 0x00)
	    {
		    //Response to set image state
		    int32_t rc = -1;
		    message.remove(0, 8);
		    blaharray.clear();
		    QCborStreamReader cbor_reader(message);
		    bool good = handleStream_state(cbor_reader, &rc, "");
//		    qDebug() << "Got " << good << ", " << rc;

//		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));
		    lbl_IMG_Status->setText("Finished.");

		    uint8_t i = 0;
		    while (i < blaharray.length())
		    {
			    qDebug() << i;
			    qDebug() << "\t" << blaharray[i].active;
			    qDebug() << "\t" << blaharray[i].bootable;
			    qDebug() << "\t" << blaharray[i].confirmed;
			    qDebug() << "\t" << blaharray[i].hash;
			    qDebug() << "\t" << blaharray[i].pending;
			    qDebug() << "\t" << blaharray[i].permanent;
			    qDebug() << "\t" << blaharray[i].image;
			    qDebug() << "\t" << blaharray[i].slot;
			    qDebug() << "\t" << blaharray[i].splitstatus;
			    qDebug() << "\t" << blaharray[i].version;
			    ++i;
		    }

		    file_list_in_progress = false;
		    emit plugin_set_status(false, false);
	    }
	    else
	    {
		    qDebug() << "Unexpected command ID: " << command;
	    }
    }
}

void plugin_mcumgr::serial_opened()
{

}

void plugin_mcumgr::serial_closed()
{
    if (file_upload_in_progress == true)
    {
	    file_upload_data.clear();
	    file_upload_in_progress = false;
	    file_upload_area = 0;
	    upload_tmr.invalidate();
	    upload_hash.clear();
    }

    if (file_list_in_progress == true)
    {
	    file_list_in_progress = false;
    }
}
