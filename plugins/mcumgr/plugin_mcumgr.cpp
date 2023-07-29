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
#include "smp_processor.h"

#include <QStandardItemModel>
#include <QRegularExpression>
#include <QClipboard>

QMainWindow *parent_window;
smp_uart *uart;
smp_processor *processor;
QStandardItemModel model_image_state;

//0x08 = new version, 0x00 = old
#define setup_smp_message(message, stream, write, group, id) \
message.append((char)((check_V2_Protocol->isChecked() ? 0x08 : 0x00) | (write == true ? 0x02 : 0x00)));  /* Read | Write (0x00 | 0x02) */ \
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

struct slot_state_t {
    uint32_t slot;
    QByteArray version;
    QByteArray hash;
    bool bootable;
    bool pending;
    bool confirmed;
    bool active;
    bool permanent;
    bool splitstatus;
    QStandardItem *item;
};

struct image_state_t {
    uint32_t image;
    bool image_set;
    QList<slot_state_t> slot_list;
    QStandardItem *item;
};

void plugin_mcumgr::setup(QMainWindow *main_window)
{
    uart = new smp_uart(this);
    processor = new smp_processor(this, uart);

	file_upload_in_progress = false;
	file_list_in_progress = false;
	file_upload_area = 0;
    shell_in_progress = false;

    parent_window = main_window;
    QTabWidget *tabWidget_orig = parent_window->findChild<QTabWidget *>("selector_Tab");
//    QPushButton *other = main_window->findChild<QPushButton *>("btn_TermClose");

//    gridLayout = new QGridLayout();
//    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
//    tabWidget = new QTabWidget(tabWidget_orig);
//    tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
    tab = new QWidget(tabWidget_orig);
    tab->setObjectName("tab");
    tabWidget_2 = new QTabWidget(tab);
    tabWidget_2->setObjectName("tabWidget_2");
    tabWidget_2->setGeometry(QRect(10, 40, 380, 281));
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
    progress_FS_Complete->setValue(0);

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
    horizontalLayout->setSpacing(2);
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
    horizontalLayout_2->setSpacing(2);
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
    horizontalLayout_3->setSpacing(2);
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
    tab_IMG_Upload = new QWidget();
    tab_IMG_Upload->setObjectName("tab_IMG_Upload");
    gridLayout_4 = new QGridLayout(tab_IMG_Upload);
    gridLayout_4->setSpacing(2);
    gridLayout_4->setObjectName("gridLayout_4");
    gridLayout_4->setContentsMargins(6, 6, 6, 6);
    progress_IMG_Complete = new QProgressBar(tab_IMG_Upload);
    progress_IMG_Complete->setObjectName("progress_IMG_Complete");
    progress_IMG_Complete->setValue(0);

    gridLayout_4->addWidget(progress_IMG_Complete, 3, 1, 1, 1);

    label_6 = new QLabel(tab_IMG_Upload);
    label_6->setObjectName("label_6");

    gridLayout_4->addWidget(label_6, 3, 0, 1, 1);

    horizontalLayout_5 = new QHBoxLayout();
    horizontalLayout_5->setSpacing(2);
    horizontalLayout_5->setObjectName("horizontalLayout_5");
    edit_IMG_Local = new QLineEdit(tab_IMG_Upload);
    edit_IMG_Local->setObjectName("edit_IMG_Local");

    horizontalLayout_5->addWidget(edit_IMG_Local);

    btn_IMG_Local = new QToolButton(tab_IMG_Upload);
    btn_IMG_Local->setObjectName("btn_IMG_Local");

    horizontalLayout_5->addWidget(btn_IMG_Local);


    gridLayout_4->addLayout(horizontalLayout_5, 0, 1, 1, 1);

    label_4 = new QLabel(tab_IMG_Upload);
    label_4->setObjectName("label_4");

    gridLayout_4->addWidget(label_4, 0, 0, 1, 1);

    horizontalLayout_4 = new QHBoxLayout();
    horizontalLayout_4->setSpacing(2);
    horizontalLayout_4->setObjectName("horizontalLayout_4");
    edit_IMG_Image = new QSpinBox(tab_IMG_Upload);
    edit_IMG_Image->setObjectName("edit_IMG_Image");
    edit_IMG_Image->setMaximumSize(QSize(60, 16777215));

    horizontalLayout_4->addWidget(edit_IMG_Image);

    radio_IMG_No_Action = new QRadioButton(tab_IMG_Upload);
    radio_IMG_No_Action->setObjectName("radio_IMG_No_Action");

    horizontalLayout_4->addWidget(radio_IMG_No_Action);

    radio_IMG_Test = new QRadioButton(tab_IMG_Upload);
    radio_IMG_Test->setObjectName("radio_IMG_Test");
    radio_IMG_Test->setChecked(true);

    horizontalLayout_4->addWidget(radio_IMG_Test);

    radio_IMG_Confirm = new QRadioButton(tab_IMG_Upload);
    radio_IMG_Confirm->setObjectName("radio_IMG_Confirm");

    horizontalLayout_4->addWidget(radio_IMG_Confirm);


    gridLayout_4->addLayout(horizontalLayout_4, 1, 1, 1, 1);

    label_41 = new QLabel(tab_IMG_Upload);
    label_41->setObjectName("label_41");

    gridLayout_4->addWidget(label_41, 1, 0, 1, 1);

    label_9 = new QLabel(tab_IMG_Upload);
    label_9->setObjectName("label_9");

    gridLayout_4->addWidget(label_9, 2, 0, 1, 1);

    check_V2_Protocol_ = new QCheckBox(tab_IMG_Upload);
    check_V2_Protocol_->setObjectName("check_V2_Protocol_");

    gridLayout_4->addWidget(check_V2_Protocol_, 2, 1, 1, 1);

    tabWidget_3->addTab(tab_IMG_Upload, QString());
    tab_IMG_Images = new QWidget();
    tab_IMG_Images->setObjectName("tab_IMG_Images");
    gridLayout_5 = new QGridLayout(tab_IMG_Images);
    gridLayout_5->setSpacing(2);
    gridLayout_5->setObjectName("gridLayout_5");
    gridLayout_5->setContentsMargins(6, 6, 6, 6);
    colview_IMG_Images = new QColumnView(tab_IMG_Images);
    colview_IMG_Images->setObjectName("colview_IMG_Images");

    gridLayout_5->addWidget(colview_IMG_Images, 0, 0, 1, 1);

    horizontalLayout_6 = new QHBoxLayout();
    horizontalLayout_6->setObjectName("horizontalLayout_6");
    label_5 = new QLabel(tab_IMG_Images);
    label_5->setObjectName("label_5");

    horizontalLayout_6->addWidget(label_5);

    radio_IMG_Get = new QRadioButton(tab_IMG_Images);
    radio_IMG_Get->setObjectName("radio_IMG_Get");
    radio_IMG_Get->setChecked(true);

    horizontalLayout_6->addWidget(radio_IMG_Get);

    radio_ING_Set = new QRadioButton(tab_IMG_Images);
    radio_ING_Set->setObjectName("radio_ING_Set");

    horizontalLayout_6->addWidget(radio_ING_Set);

    horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_6->addItem(horizontalSpacer_5);


    gridLayout_5->addLayout(horizontalLayout_6, 1, 0, 1, 1);

    tabWidget_3->addTab(tab_IMG_Images, QString());
    tab_IMG_Erase = new QWidget();
    tab_IMG_Erase->setObjectName("tab_IMG_Erase");
    gridLayout_10 = new QGridLayout(tab_IMG_Erase);
    gridLayout_10->setObjectName("gridLayout_10");
    label_14 = new QLabel(tab_IMG_Erase);
    label_14->setObjectName("label_14");

    gridLayout_10->addWidget(label_14, 0, 0, 1, 1);

    horizontalSpacer_9 = new QSpacerItem(235, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    gridLayout_10->addItem(horizontalSpacer_9, 0, 2, 1, 1);

    edit_IMG_Erase_Slot = new QSpinBox(tab_IMG_Erase);
    edit_IMG_Erase_Slot->setObjectName("edit_IMG_Erase_Slot");
    edit_IMG_Erase_Slot->setMinimumSize(QSize(40, 0));
    edit_IMG_Erase_Slot->setMaximumSize(QSize(16777215, 16777215));

    gridLayout_10->addWidget(edit_IMG_Erase_Slot, 0, 1, 1, 1);

    verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    gridLayout_10->addItem(verticalSpacer_2, 1, 0, 1, 1);

    tabWidget_3->addTab(tab_IMG_Erase, QString());

    gridLayout_3->addWidget(tabWidget_3, 0, 0, 1, 1);

    tabWidget_2->addTab(tab_3, QString());
    tab_OS = new QWidget();
    tab_OS->setObjectName("tab_OS");
    gridLayout_7 = new QGridLayout(tab_OS);
    gridLayout_7->setSpacing(2);
    gridLayout_7->setObjectName("gridLayout_7");
    selector_OS = new QTabWidget(tab_OS);
    selector_OS->setObjectName("selector_OS");
    tab_OS_Echo = new QWidget();
    tab_OS_Echo->setObjectName("tab_OS_Echo");
    gridLayout_8 = new QGridLayout(tab_OS_Echo);
    gridLayout_8->setObjectName("gridLayout_8");
    label_10 = new QLabel(tab_OS_Echo);
    label_10->setObjectName("label_10");

    gridLayout_8->addWidget(label_10, 0, 0, 1, 1);

    edit_OS_Echo_Input = new QPlainTextEdit(tab_OS_Echo);
    edit_OS_Echo_Input->setObjectName("edit_OS_Echo_Input");

    gridLayout_8->addWidget(edit_OS_Echo_Input, 0, 1, 1, 1);

    label_11 = new QLabel(tab_OS_Echo);
    label_11->setObjectName("label_11");

    gridLayout_8->addWidget(label_11, 1, 0, 1, 1);

    edit_OS_Echo_Output = new QPlainTextEdit(tab_OS_Echo);
    edit_OS_Echo_Output->setObjectName("edit_OS_Echo_Output");
    edit_OS_Echo_Output->setUndoRedoEnabled(false);
    edit_OS_Echo_Output->setReadOnly(true);

    gridLayout_8->addWidget(edit_OS_Echo_Output, 1, 1, 1, 1);

    selector_OS->addTab(tab_OS_Echo, QString());
    tab_8 = new QWidget();
    tab_8->setObjectName("tab_8");
    selector_OS->addTab(tab_8, QString());
    tab_11 = new QWidget();
    tab_11->setObjectName("tab_11");
    selector_OS->addTab(tab_11, QString());
    tab_OS_Reset = new QWidget();
    tab_OS_Reset->setObjectName("tab_OS_Reset");
    gridLayout_12 = new QGridLayout(tab_OS_Reset);
    gridLayout_12->setObjectName("gridLayout_12");
    check_OS_Force_Reboot = new QCheckBox(tab_OS_Reset);
    check_OS_Force_Reboot->setObjectName("check_OS_Force_Reboot");

    gridLayout_12->addWidget(check_OS_Force_Reboot, 0, 0, 1, 1);

    verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    gridLayout_12->addItem(verticalSpacer_3, 1, 0, 1, 1);

    selector_OS->addTab(tab_OS_Reset, QString());
    tab_OS_Info = new QWidget();
    tab_OS_Info->setObjectName("tab_OS_Info");
    gridLayout_13 = new QGridLayout(tab_OS_Info);
    gridLayout_13->setSpacing(2);
    gridLayout_13->setObjectName("gridLayout_13");
    label_17 = new QLabel(tab_OS_Info);
    label_17->setObjectName("label_17");

    gridLayout_13->addWidget(label_17, 0, 0, 1, 1);

    edit_OS_UNam = new QLineEdit(tab_OS_Info);
    edit_OS_UNam->setObjectName("edit_OS_UNam");
    edit_OS_UNam->setReadOnly(true);

    gridLayout_13->addWidget(edit_OS_UNam, 0, 1, 1, 1);

    horizontalLayout_10 = new QHBoxLayout();
    horizontalLayout_10->setSpacing(2);
    horizontalLayout_10->setObjectName("horizontalLayout_10");
    radio_FS_Upload_2 = new QRadioButton(tab_OS_Info);
    radio_FS_Upload_2->setObjectName("radio_FS_Upload_2");
    radio_FS_Upload_2->setChecked(true);

    horizontalLayout_10->addWidget(radio_FS_Upload_2);

    radio_FS_Download_2 = new QRadioButton(tab_OS_Info);
    radio_FS_Download_2->setObjectName("radio_FS_Download_2");

    horizontalLayout_10->addWidget(radio_FS_Download_2);


    gridLayout_13->addLayout(horizontalLayout_10, 1, 0, 1, 2);

    label_18 = new QLabel(tab_OS_Info);
    label_18->setObjectName("label_18");

    gridLayout_13->addWidget(label_18, 2, 0, 1, 1);

    edit_OS_Info_Output = new QPlainTextEdit(tab_OS_Info);
    edit_OS_Info_Output->setObjectName("edit_OS_Info_Output");

    gridLayout_13->addWidget(edit_OS_Info_Output, 2, 1, 1, 1);

    selector_OS->addTab(tab_OS_Info, QString());

    gridLayout_7->addWidget(selector_OS, 0, 0, 1, 1);

    horizontalLayout_13 = new QHBoxLayout();
    horizontalLayout_13->setSpacing(2);
    horizontalLayout_13->setObjectName("horizontalLayout_13");
    horizontalLayout_13->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_17 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_13->addItem(horizontalSpacer_17);

    btn_OS_Go = new QPushButton(tab_OS);
    btn_OS_Go->setObjectName("btn_OS_Go");

    horizontalLayout_13->addWidget(btn_OS_Go);

    horizontalSpacer_18 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_13->addItem(horizontalSpacer_18);


    gridLayout_7->addLayout(horizontalLayout_13, 2, 0, 1, 1);

    lbl_OS_Status = new QLabel(tab_OS);
    lbl_OS_Status->setObjectName("lbl_OS_Status");

    gridLayout_7->addWidget(lbl_OS_Status, 1, 0, 1, 1);

    tabWidget_2->addTab(tab_OS, QString());
    tab_5 = new QWidget();
    tab_5->setObjectName("tab_5");
    gridLayout_11 = new QGridLayout(tab_5);
    gridLayout_11->setObjectName("gridLayout_11");
    label_15 = new QLabel(tab_5);
    label_15->setObjectName("label_15");

    gridLayout_11->addWidget(label_15, 0, 0, 1, 1);

    combo_STAT_Group = new QComboBox(tab_5);
    combo_STAT_Group->setObjectName("combo_STAT_Group");
    combo_STAT_Group->setEditable(true);

    gridLayout_11->addWidget(combo_STAT_Group, 0, 1, 1, 1);

    label_16 = new QLabel(tab_5);
    label_16->setObjectName("label_16");

    gridLayout_11->addWidget(label_16, 1, 0, 1, 1);

    table_STAT_Values = new QTableWidget(tab_5);
    table_STAT_Values->setObjectName("table_STAT_Values");

    gridLayout_11->addWidget(table_STAT_Values, 1, 1, 1, 1);

    horizontalLayout_9 = new QHBoxLayout();
    horizontalLayout_9->setSpacing(2);
    horizontalLayout_9->setObjectName("horizontalLayout_9");
    radio_STAT_List = new QRadioButton(tab_5);
    radio_STAT_List->setObjectName("radio_STAT_List");
    radio_STAT_List->setChecked(true);

    horizontalLayout_9->addWidget(radio_STAT_List);

    radio_STAT_Fetch = new QRadioButton(tab_5);
    radio_STAT_Fetch->setObjectName("radio_STAT_Fetch");

    horizontalLayout_9->addWidget(radio_STAT_Fetch);


    gridLayout_11->addLayout(horizontalLayout_9, 2, 0, 1, 2);

    lbl_STAT_Status = new QLabel(tab_5);
    lbl_STAT_Status->setObjectName("lbl_STAT_Status");

    gridLayout_11->addWidget(lbl_STAT_Status, 3, 0, 1, 2);

    horizontalLayout_14 = new QHBoxLayout();
    horizontalLayout_14->setSpacing(2);
    horizontalLayout_14->setObjectName("horizontalLayout_14");
    horizontalLayout_14->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_19 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_14->addItem(horizontalSpacer_19);

    btn_STAT_Go = new QPushButton(tab_5);
    btn_STAT_Go->setObjectName("btn_STAT_Go");

    horizontalLayout_14->addWidget(btn_STAT_Go);

    horizontalSpacer_20 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_14->addItem(horizontalSpacer_20);


    gridLayout_11->addLayout(horizontalLayout_14, 4, 0, 1, 2);

    tabWidget_2->addTab(tab_5, QString());
    tab_6 = new QWidget();
    tab_6->setObjectName("tab_6");
    gridLayout_9 = new QGridLayout(tab_6);
    gridLayout_9->setSpacing(2);
    gridLayout_9->setObjectName("gridLayout_9");
    edit_SHELL_Input = new QLineEdit(tab_6);
    edit_SHELL_Input->setObjectName("edit_SHELL_Input");

    gridLayout_9->addWidget(edit_SHELL_Input, 0, 1, 1, 2);

    horizontalLayout_8 = new QHBoxLayout();
    horizontalLayout_8->setSpacing(2);
    horizontalLayout_8->setObjectName("horizontalLayout_8");
    horizontalLayout_8->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_8->addItem(horizontalSpacer_7);

    btn_SHELL_Go = new QPushButton(tab_6);
    btn_SHELL_Go->setObjectName("btn_SHELL_Go");

    horizontalLayout_8->addWidget(btn_SHELL_Go);

    horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_8->addItem(horizontalSpacer_8);


    gridLayout_9->addLayout(horizontalLayout_8, 3, 0, 1, 3);

    edit_SHELL_Log = new QPlainTextEdit(tab_6);
    edit_SHELL_Log->setObjectName("edit_SHELL_Log");
    edit_SHELL_Log->setUndoRedoEnabled(false);
    edit_SHELL_Log->setReadOnly(true);

    gridLayout_9->addWidget(edit_SHELL_Log, 4, 0, 1, 3);

    lbl_SHELL_Status = new QLabel(tab_6);
    lbl_SHELL_Status->setObjectName("lbl_SHELL_Status");

    gridLayout_9->addWidget(lbl_SHELL_Status, 2, 0, 1, 3);

    label_13 = new QLabel(tab_6);
    label_13->setObjectName("label_13");

    gridLayout_9->addWidget(label_13, 1, 0, 1, 1);

    edit_SHELL_Output = new QPlainTextEdit(tab_6);
    edit_SHELL_Output->setObjectName("edit_SHELL_Output");
    edit_SHELL_Output->setUndoRedoEnabled(false);
    edit_SHELL_Output->setReadOnly(true);

    gridLayout_9->addWidget(edit_SHELL_Output, 1, 1, 1, 1);

    label_12 = new QLabel(tab_6);
    label_12->setObjectName("label_12");

    gridLayout_9->addWidget(label_12, 0, 0, 1, 1);

    verticalLayout_3 = new QVBoxLayout();
    verticalLayout_3->setObjectName("verticalLayout_3");
    btn_SHELL_Clear = new QToolButton(tab_6);
    btn_SHELL_Clear->setObjectName("btn_SHELL_Clear");

    verticalLayout_3->addWidget(btn_SHELL_Clear);

    btn_SHELL_Copy = new QToolButton(tab_6);
    btn_SHELL_Copy->setObjectName("btn_SHELL_Copy");

    verticalLayout_3->addWidget(btn_SHELL_Copy);


    gridLayout_9->addLayout(verticalLayout_3, 1, 2, 1, 1);

    tabWidget_2->addTab(tab_6, QString());
//    verticalLayoutWidget = new QWidget(tab);
        verticalLayoutWidget = new QWidget();
    verticalLayoutWidget->setObjectName("verticalLayoutWidget");
//    verticalLayoutWidget->setGeometry(QRect(410, 20, 229, 182));
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
    edit_IMG_Preview_Hash->setReadOnly(true);

    formLayout->setWidget(0, QFormLayout::FieldRole, edit_IMG_Preview_Hash);

    label_8 = new QLabel(verticalLayoutWidget);
    label_8->setObjectName("label_8");

    formLayout->setWidget(1, QFormLayout::LabelRole, label_8);

    edit_IMG_Preview_Version = new QLineEdit(verticalLayoutWidget);
    edit_IMG_Preview_Version->setObjectName("edit_IMG_Preview_Version");
    edit_IMG_Preview_Version->setReadOnly(true);

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

    horizontalLayoutWidget = new QWidget(tab);
    horizontalLayoutWidget->setObjectName("horizontalLayoutWidget");
    horizontalLayoutWidget->setGeometry(QRect(10, 10, 341, 31));
    horizontalLayout_7 = new QHBoxLayout(horizontalLayoutWidget);
    horizontalLayout_7->setObjectName("horizontalLayout_7");
    horizontalLayout_7->setContentsMargins(0, 0, 0, 0);
    label = new QLabel(horizontalLayoutWidget);
    label->setObjectName("label");

    horizontalLayout_7->addWidget(label);

    edit_MTU = new QSpinBox(horizontalLayoutWidget);
    edit_MTU->setObjectName("edit_MTU");
    edit_MTU->setMinimum(32);
    edit_MTU->setMaximum(2048);
    edit_MTU->setValue(128);

    horizontalLayout_7->addWidget(edit_MTU);

    check_V2_Protocol = new QCheckBox(horizontalLayoutWidget);
    check_V2_Protocol->setObjectName("check_V2_Protocol");

    horizontalLayout_7->addWidget(check_V2_Protocol);

    horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_7->addItem(horizontalSpacer_6);

//    tabWidget->addTab(tab, QString());

//    gridLayout->addWidget(tabWidget, 0, 0, 1, 1);


//    retranslateUi(Form);

//    tabWidget->setCurrentIndex(0);
    tabWidget_2->setCurrentIndex(2);
    tabWidget_3->setCurrentIndex(0);
    selector_OS->setCurrentIndex(4);

//    QMetaObject::connectSlotsByName(Form);

    //retranslate code
//    Form->setWindowTitle(QCoreApplication::translate("Form", "Form", nullptr));
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
    label_6->setText(QCoreApplication::translate("Form", "Progress:", nullptr));
    btn_IMG_Local->setText(QCoreApplication::translate("Form", "...", nullptr));
    label_4->setText(QCoreApplication::translate("Form", "File:", nullptr));
    radio_IMG_No_Action->setText(QCoreApplication::translate("Form", "No action", nullptr));
    radio_IMG_Test->setText(QCoreApplication::translate("Form", "Test", nullptr));
    radio_IMG_Confirm->setText(QCoreApplication::translate("Form", "Confirm", nullptr));
    label_41->setText(QCoreApplication::translate("Form", "Image:", nullptr));
    label_9->setText(QCoreApplication::translate("Form", "Reset:", nullptr));
    check_V2_Protocol_->setText(QCoreApplication::translate("Form", "After upload", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Upload), QCoreApplication::translate("Form", "Upload", nullptr));
    label_5->setText(QCoreApplication::translate("Form", "State:", nullptr));
    radio_IMG_Get->setText(QCoreApplication::translate("Form", "Get", nullptr));
    radio_ING_Set->setText(QCoreApplication::translate("Form", "Set", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Images), QCoreApplication::translate("Form", "Images", nullptr));
    label_14->setText(QCoreApplication::translate("Form", "Slot:", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Erase), QCoreApplication::translate("Form", "Erase", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_3), QCoreApplication::translate("Form", "Img", nullptr));
    label_10->setText(QCoreApplication::translate("Form", "Input:", nullptr));
    label_11->setText(QCoreApplication::translate("Form", "Output:", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Echo), QCoreApplication::translate("Form", "Echo", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_8), QCoreApplication::translate("Form", "Stats", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_11), QCoreApplication::translate("Form", "Memory", nullptr));
    check_OS_Force_Reboot->setText(QCoreApplication::translate("Form", "Force reboot", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Reset), QCoreApplication::translate("Form", "Reset", nullptr));
    label_17->setText(QCoreApplication::translate("Form", "uname:", nullptr));
    radio_FS_Upload_2->setText(QCoreApplication::translate("Form", "Buffer info", nullptr));
    radio_FS_Download_2->setText(QCoreApplication::translate("Form", "uname", nullptr));
    label_18->setText(QCoreApplication::translate("Form", "Output:", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Info), QCoreApplication::translate("Form", "Info", nullptr));
    btn_OS_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    lbl_OS_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_OS), QCoreApplication::translate("Form", "OS", nullptr));
    label_15->setText(QCoreApplication::translate("Form", "Group:", nullptr));
    label_16->setText(QCoreApplication::translate("Form", "Values:", nullptr));
    radio_STAT_List->setText(QCoreApplication::translate("Form", "List Groups", nullptr));
    radio_STAT_Fetch->setText(QCoreApplication::translate("Form", "Fetch Stats", nullptr));
    lbl_STAT_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    btn_STAT_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_5), QCoreApplication::translate("Form", "Stats", nullptr));
    btn_SHELL_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    lbl_SHELL_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    label_13->setText(QCoreApplication::translate("Form", "Output:", nullptr));
    label_12->setText(QCoreApplication::translate("Form", "Input:", nullptr));
    btn_SHELL_Clear->setText(QCoreApplication::translate("Form", "Clear", nullptr));
    btn_SHELL_Copy->setText(QCoreApplication::translate("Form", "Copy", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_6), QCoreApplication::translate("Form", "Shell", nullptr));
    label_7->setText(QCoreApplication::translate("Form", "Hash:", nullptr));
    label_8->setText(QCoreApplication::translate("Form", "Version:", nullptr));
    check_IMG_Preview_Confirmed->setText(QCoreApplication::translate("Form", "Confirmed", nullptr));
    check_IMG_Preview_Active->setText(QCoreApplication::translate("Form", "Active", nullptr));
    check_IMG_Preview_Pending->setText(QCoreApplication::translate("Form", "Pending", nullptr));
    check_IMG_Preview_Bootable->setText(QCoreApplication::translate("Form", "Bootable", nullptr));
    check_IMG_Preview_Permanent->setText(QCoreApplication::translate("Form", "Permanent", nullptr));
    btn_IMG_Preview_Copy->setText(QCoreApplication::translate("Form", "Copy", nullptr));
    label->setText(QCoreApplication::translate("Form", "MTU:", nullptr));
    check_V2_Protocol->setText(QCoreApplication::translate("Form", "v2 protocol", nullptr));
//    tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("Form", "MCUmgr", nullptr));

    //Add code
    tabWidget_orig->addTab(tab, QString("MCUmgr"));

    //QMetaObject::connectSlotsByName(Form);

//Signals
    connect(uart, SIGNAL(serial_write(QByteArray*)), parent_window, SLOT(plugin_serial_transmit(QByteArray*)));
    //connect(uart, SIGNAL(receive_waiting(QByteArray)), this, SLOT(receive_waiting(QByteArray)));
    connect(uart, SIGNAL(receive_waiting(smp_message*)), processor, SLOT(message_received(smp_message*)));
    //connect(btn_IMG_Local, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Local_clicked()));
    //connect(btn_IMG_Go, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Go_clicked()));
    connect(processor, SIGNAL(receive_error(uint8_t,uint8_t,uint16_t,uint8_t,smp_error_t)), this, SLOT(receive_error(uint8_t,uint8_t,uint16_t,uint8_t,smp_error_t)));
    connect(processor, SIGNAL(receive_ok(uint8_t,uint8_t,uint16_t,uint8_t,QByteArray*)), this, SLOT(receive_ok(uint8_t,uint8_t,uint16_t,uint8_t,QByteArray*)));

//Form signals
    connect(btn_FS_Local, SIGNAL(clicked()), this, SLOT(on_btn_FS_Local_clicked()));
    connect(btn_FS_Go, SIGNAL(clicked()), this, SLOT(on_btn_FS_Go_clicked()));
    connect(radio_FS_Upload, SIGNAL(clicked()), this, SLOT(on_radio_FS_Upload_clicked()));
    connect(radio_FS_Download, SIGNAL(clicked()), this, SLOT(on_radio_FS_Download_clicked()));
    connect(radio_FS_Size, SIGNAL(clicked()), this, SLOT(on_radio_FS_Size_clicked()));
    connect(radio_FS_HashChecksum, SIGNAL(clicked()), this, SLOT(on_radio_FS_HashChecksum_clicked()));
    connect(btn_IMG_Local, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Local_clicked()));
    connect(btn_IMG_Go, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Go_clicked()));
    connect(radio_IMG_No_Action, SIGNAL(toggled(bool)), this, SLOT(on_radio_IMG_No_Action_toggled(bool)));
    connect(btn_IMG_Preview_Copy, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Preview_Copy_clicked()));
    connect(btn_OS_Go, SIGNAL(clicked()), this, SLOT(on_btn_OS_Go_clicked()));
    connect(btn_SHELL_Go, SIGNAL(clicked()), this, SLOT(on_btn_SHELL_Go_clicked()));
    connect(btn_STAT_Go, SIGNAL(clicked()), this, SLOT(on_btn_STAT_Go_clicked()));
    connect(btn_SHELL_Clear, SIGNAL(clicked()), this, SLOT(on_btn_SHELL_Clear_clicked()));
    connect(btn_SHELL_Copy, SIGNAL(clicked()), this, SLOT(on_btn_SHELL_Copy_clicked()));

connect(colview_IMG_Images, SIGNAL(updatePreviewWidget(QModelIndex)), this, SLOT(on_colview_IMG_Images_updatePreviewWidget(QModelIndex)));
    colview_IMG_Images->setModel(&model_image_state);

    check_IMG_Preview_Confirmed->setChecked(true);
    check_IMG_Preview_Confirmed->installEventFilter(this);
    check_IMG_Preview_Active->installEventFilter(this);
    check_IMG_Preview_Pending->installEventFilter(this);
    check_IMG_Preview_Bootable->installEventFilter(this);
    check_IMG_Preview_Permanent->installEventFilter(this);


    //test
    emit plugin_add_open_close_button(btn_FS_Go);
}

plugin_mcumgr::~plugin_mcumgr()
{
    delete processor;
    delete uart;
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
    return "AuTerm MCUmgr plugin\r\nCopyright 2021-2023 Jamie M.\r\n\r\nCan be used to communicate with Zephyr devices with the serial MCUmgr transport enabled.\r\n\r\nUNFINISHED INITIAL TEST USE ONLY, NOT REPRESENTATIVE OF FINAL PRODUCT.\r\n\r\nBuilt using Qt " QT_VERSION_STR;
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

QList<image_state_t> blaharray;
image_state_t thisblah;
slot_state_t thisblah2;

bool plugin_mcumgr::handleStream_state(QCborStreamReader &reader, int32_t *new_rc, QString array_name)
{
QString array_name_dupe = array_name;
    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    int32_t rc = -1;
    int64_t off = -1;

    thisblah.image = 0;
    thisblah.image_set = false;
    thisblah.slot_list.clear();
    thisblah.item = nullptr;
    thisblah2.slot = 0;
    thisblah2.version.clear();
    thisblah2.hash.clear();
    thisblah2.bootable = false;
    thisblah2.pending = false;
    thisblah2.confirmed = false;
    thisblah2.active = false;
    thisblah2.permanent = false;
    thisblah2.splitstatus = false;
    thisblah2.item = nullptr;
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
                index = &thisblah2.bootable;
		    }
		    else if (key == "pending")
		    {
                index = &thisblah2.pending;
		    }
		    else if (key == "confirmed")
		    {
                index = &thisblah2.confirmed;
		    }
		    else if (key == "active")
		    {
                index = &thisblah2.active;
		    }
		    else if (key == "permanent")
		    {
                index = &thisblah2.permanent;
		    }
		    else if (key == "splitStatus")
		    {
                index = &thisblah2.splitstatus;
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
                thisblah.image_set = true;
		    }
		    else if (key == "slot")
		    {
                thisblah2.slot = reader.toUnsignedInteger();
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
                thisblah2.hash = data;
                emit plugin_to_hex(&thisblah2.hash);
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
                    thisblah2.version = data.toUtf8();
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
                    image_state_t *image_state_ptr = nullptr;

                    if (blaharray.length() > 0)
                    {
                        uint8_t i = 0;
                        while (i < blaharray.length())
                        {
                            if (blaharray.at(i).image_set == thisblah.image_set && (thisblah.image_set == false || blaharray.at(i).image == thisblah.image))
                            {
                                image_state_ptr = &blaharray[i];
                                break;
                            }

                            ++i;
                        }
                    }

                    if (image_state_ptr == nullptr)
                    {
                        if (thisblah.image_set == true)
                        {
                            thisblah.item = new QStandardItem(QString("Image ").append(QString::number(thisblah.image)));
                        }
                        else
                        {
                            thisblah.item = new QStandardItem("Images");
                        }
                        blaharray.append(thisblah);
                        image_state_ptr = &blaharray.last();
                        model_image_state.appendRow(thisblah.item);
                    }

                    thisblah2.item = new QStandardItem(QString("Slot ").append(QString::number(thisblah2.slot)));
                    image_state_ptr->slot_list.append(thisblah2);
                    image_state_ptr->item->appendRow(thisblah2.item);
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

bool plugin_mcumgr::handleStream_shell(QCborStreamReader &reader, int32_t *new_rc, int32_t *new_ret, QString *new_data)
{
//    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    int32_t rc = -1;
    int32_t ret = -1;

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
            else if (key == "ret")
            {
//			    qDebug() << "found off";
                ret = reader.toInteger();
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
                else if (key == "o")
                {
                    new_data->append(data);
                }
            }
            break;
        }
        case QCborStreamReader::Array:
        case QCborStreamReader::Map:
            reader.enterContainer();
            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                handleStream_shell(reader, new_rc, new_ret, new_data);
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

    if (new_ret != NULL && ret != -1)
    {
        *new_ret = ret;
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
//    message->remove(0, 8);
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

                smp_message *tmp_message = new smp_message();
                tmp_message->append(message);
                processor->send(tmp_message, 4000, 3);

                //uart->send(&message);
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

        smp_message *tmp_message = new smp_message();
        tmp_message->append(message);
        processor->send(tmp_message, 4000, 3);
        //uart->send(&message);
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
		    QCborStreamReader cbor_reader(message);
		    bool good = handleStream_state(cbor_reader, &rc, "");
//		    qDebug() << "Got " << good << ", " << rc;

//		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));
		    lbl_IMG_Status->setText("Finished.");

#if 0
            uint8_t i = 0;
		    while (i < blaharray.length())
		    {
			    qDebug() << i;
                qDebug() << "\t" << blaharray[i].image;
                qDebug() << "\t" << blaharray[i].image_set;

                edit_IMG_Log->appendPlainText(QString::number(i));
                edit_IMG_Log->appendPlainText(QString::number(blaharray[i].image));
                edit_IMG_Log->appendPlainText(QString::number(blaharray[i].image_set));
                edit_IMG_Log->appendPlainText(QString::number(blaharray[i].slot_list.length()));

                uint8_t l = 0;
                while (l < blaharray[i].slot_list.length())
                {
                    qDebug() << "\t" << blaharray[i].slot_list[l].slot;
                    qDebug() << "\t" << blaharray[i].slot_list[l].active;
                    qDebug() << "\t" << blaharray[i].slot_list[l].bootable;
                    qDebug() << "\t" << blaharray[i].slot_list[l].confirmed;
                    qDebug() << "\t" << blaharray[i].slot_list[l].hash;
                    qDebug() << "\t" << blaharray[i].slot_list[l].pending;
                    qDebug() << "\t" << blaharray[i].slot_list[l].permanent;
                    qDebug() << "\t" << blaharray[i].slot_list[l].splitstatus;
                    qDebug() << "\t" << blaharray[i].slot_list[l].version;
                    edit_IMG_Log->appendPlainText(QString::number(blaharray[i].slot_list[l].slot));
                    edit_IMG_Log->appendPlainText(QString::number(blaharray[i].slot_list[l].active));
                    edit_IMG_Log->appendPlainText(QString::number(blaharray[i].slot_list[l].bootable));
                    edit_IMG_Log->appendPlainText(QString::number(blaharray[i].slot_list[l].confirmed));
                    edit_IMG_Log->appendPlainText(QString::number(blaharray[i].slot_list[l].pending));
                    edit_IMG_Log->appendPlainText(QString::number(blaharray[i].slot_list[l].permanent));
                    edit_IMG_Log->appendPlainText(QString::number(blaharray[i].slot_list[l].splitstatus));
                    edit_IMG_Log->appendPlainText(QString(blaharray[i].slot_list[l].version));
                    edit_IMG_Log->appendPlainText(QString(blaharray[i].slot_list[l].hash));
                    ++l;
                }

			    ++i;
		    }
#endif

		    file_list_in_progress = false;
		    emit plugin_set_status(false, false);
	    }
	    else
	    {
		    qDebug() << "Unexpected command ID: " << command;
	    }
    }
    else if (shell_in_progress == true && group == 9 && command == 0)
    {
        int32_t rc = -1;
        int32_t ret = -1;
        QString data;
        message.remove(0, 8);
        QCborStreamReader cbor_reader(message);
        bool good = handleStream_shell(cbor_reader, &rc, &ret, &data);

        shell_in_progress = false;
        emit plugin_set_status(false, false);

        if (rc != -1)
        {
            lbl_SHELL_Status->setText(QString("Finished, error (rc): ").append(QString::number(rc)));
        }
        else if (ret != -1)
        {
            lbl_SHELL_Status->setText(QString("Finished, error (ret): ").append(QString::number(ret)));
        }
        else
        {
            lbl_SHELL_Status->setText("Finished");
        }

        edit_SHELL_Output->appendPlainText(data);
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

//Form actions
void plugin_mcumgr::on_btn_FS_Local_clicked()
{
}

void plugin_mcumgr::on_btn_FS_Go_clicked()
{
}

void plugin_mcumgr::on_radio_FS_Upload_clicked()
{
}

void plugin_mcumgr::on_radio_FS_Download_clicked()
{
}

void plugin_mcumgr::on_radio_FS_Size_clicked()
{
}

void plugin_mcumgr::on_radio_FS_HashChecksum_clicked()
{
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


        smp_message *tmp_message = new smp_message();
        tmp_message->append(message);

//	    qDebug() << "len: " << message.length();

//        uart->send(&message);
        processor->send(tmp_message, 4000, 3);

        progress_IMG_Complete->setValue(0);
        lbl_IMG_Status->setText("Uploading...");
    }
    else if (tabWidget_3->currentIndex() == 1)
    {
        //Image list
        emit plugin_set_status(true, false);

        colview_IMG_Images->previewWidget()->hide();
        model_image_state.clear();
        blaharray.clear();

        QByteArray message;
        QByteArray smp_data;
        QCborStreamWriter smp_stream(&smp_data);

        setup_smp_message(message, smp_stream, false, 0x01, 0x00);
        finish_smp_message(message, smp_stream, smp_data);

        file_list_in_progress = true;

//	    qDebug() << "len: " << message.length();

        smp_message *tmp_message = new smp_message();
        tmp_message->append(message);
        processor->send(tmp_message, 4000, 3);
//        uart->send(&message);

        progress_IMG_Complete->setValue(0);
        lbl_IMG_Status->setText("Querying...");
    }
}

void plugin_mcumgr::on_radio_IMG_No_Action_toggled(bool checked)
{
}

void plugin_mcumgr::on_btn_IMG_Preview_Copy_clicked()
{
}

void plugin_mcumgr::on_btn_OS_Go_clicked()
{
    if (selector_OS->currentWidget() == tab_OS_Echo)
    {
        edit_OS_Echo_Output->appendPlainText("Echo");
    }
    else if (selector_OS->currentWidget() == tab_OS_Reset)
    {
        edit_OS_Echo_Output->appendPlainText("Reset");
    }
    else if (selector_OS->currentWidget() == tab_OS_Info)
    {
        edit_OS_Echo_Output->appendPlainText("Info");
    }
}

void plugin_mcumgr::on_btn_SHELL_Go_clicked()
{
        //Execute shell command
        emit plugin_set_status(true, false);

        QByteArray message;
        QByteArray smp_data;
        QCborStreamWriter smp_stream(&smp_data);

        setup_smp_message(message, smp_stream, true, 0x09, 0x00);

        smp_stream.append("argv");
        smp_stream.startArray();

        QRegularExpression reTempRE("\\s+");
        QStringList list_arguments = edit_SHELL_Input->text().split(reTempRE);

        uint8_t i = 0;
        while (i < list_arguments.length())
        {
            smp_stream.append(list_arguments.at(i));
            ++i;
        }

        smp_stream.endArray();

        finish_smp_message(message, smp_stream, smp_data);

        smp_message *tmp_message = new smp_message();
        tmp_message->append(message);

        shell_in_progress = true;

//	    qDebug() << "len: " << message.length();

//        uart->send(&message);
        processor->send(tmp_message, 4000, 3);

        lbl_SHELL_Status->setText("Executing...");
}

void plugin_mcumgr::on_btn_STAT_Go_clicked()
{
    if (radio_STAT_List->isChecked())
    {
    }
    else if (radio_STAT_Fetch->isChecked())
    {
    }
}

void plugin_mcumgr::on_btn_SHELL_Clear_clicked()
{
    edit_SHELL_Output->clear();
}

void plugin_mcumgr::on_btn_SHELL_Copy_clicked()
{
    QApplication::clipboard()->setText(edit_SHELL_Output->toPlainText());
}

void plugin_mcumgr::on_colview_IMG_Images_updatePreviewWidget(const QModelIndex &index)
{
    uint8_t i = 0;
    while (i < blaharray.length())
    {
        if (blaharray[i].item == model_image_state.itemFromIndex(index)->parent())
        {
            uint8_t l = 0;
            while (l < blaharray[i].slot_list.length())
            {
                if (model_image_state.itemFromIndex(index) == blaharray[i].slot_list[l].item)
                {
                    edit_IMG_Preview_Hash->setText(blaharray[i].slot_list[l].hash);
                    edit_IMG_Preview_Version->setText(blaharray[i].slot_list[l].version);
                    check_IMG_Preview_Active->setChecked(blaharray[i].slot_list[l].active);
                    check_IMG_Preview_Bootable->setChecked(blaharray[i].slot_list[l].bootable);
                    check_IMG_Preview_Confirmed->setChecked(blaharray[i].slot_list[l].confirmed);
                    check_IMG_Preview_Pending->setChecked(blaharray[i].slot_list[l].pending);
                    check_IMG_Preview_Permanent->setChecked(blaharray[i].slot_list[l].permanent);

                    i = 99;
                    break;
                }

                ++l;
            }
        }

        ++i;
    }

    if (colview_IMG_Images->previewWidget() != verticalLayoutWidget)
    {
        colview_IMG_Images->setPreviewWidget(verticalLayoutWidget);
    }
    else
    {
        colview_IMG_Images->previewWidget()->show();
    }
}

void plugin_mcumgr::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray *data)
{
    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;


    if (file_upload_in_progress == true && group == 1)
    {
        if (command == 0x00)
        {
            //Response to set image state
            int32_t rc = -1;
//            message.remove(0, 8);
            QCborStreamReader cbor_reader(*data);
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
            file_upload(data);
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
//            message.remove(0, 8);
            QCborStreamReader cbor_reader(*data);
            bool good = handleStream_state(cbor_reader, &rc, "");
            //		    qDebug() << "Got " << good << ", " << rc;

            //		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));
            lbl_IMG_Status->setText("Finished.");

            file_list_in_progress = false;
            emit plugin_set_status(false, false);
        }
        else
        {
            qDebug() << "Unexpected command ID: " << command;
        }
    }
    else if (shell_in_progress == true && group == 9 && command == 0)
    {
        int32_t rc = -1;
        int32_t ret = -1;
        QString response;
        QCborStreamReader cbor_reader(*data);
        bool good = handleStream_shell(cbor_reader, &rc, &ret, &response);

        shell_in_progress = false;
        emit plugin_set_status(false, false);

        if (rc != -1)
        {
            lbl_SHELL_Status->setText(QString("Finished, error (rc): ").append(QString::number(rc)));
        }
        else if (ret != -1)
        {
            lbl_SHELL_Status->setText(QString("Finished, error (ret): ").append(QString::number(ret)));
        }
        else
        {
            lbl_SHELL_Status->setText("Finished");
        }

        edit_SHELL_Output->appendPlainText(response);
    }
}

void plugin_mcumgr::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    qDebug() << "Got error: " << version << ", " << op << ", " << group << ", "  << command << ", " << error.type << ", " << error.rc << ", " << error.group;
}
