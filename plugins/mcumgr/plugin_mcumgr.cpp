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
#include "smp_group_img_mgmt.h"
#include "smp_group_os_mgmt.h"
#include "smp_error.h"

#include <QStandardItemModel>
#include <QRegularExpression>
#include <QClipboard>

const uint8_t retries = 3;
const uint16_t timeout_ms = 3000;
const uint16_t timeout_erase_ms = 14000;

QMainWindow *parent_window;
smp_uart *uart;
smp_processor *processor;
QStandardItemModel model_image_state;
smp_group_img_mgmt *my_img;
smp_group_os_mgmt *my_os;
QList<image_state_t> blaharray;

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
    tab->setObjectName(QString::fromUtf8("tab"));
    tabWidget_2 = new QTabWidget(tab);
    tabWidget_2->setObjectName(QString::fromUtf8("tabWidget_2"));
    tabWidget_2->setGeometry(QRect(10, 40, 380, 281));
    tabWidget_2->setTabPosition(QTabWidget::West);
    tab_2 = new QWidget();
    tab_2->setObjectName(QString::fromUtf8("tab_2"));
    gridLayout_2 = new QGridLayout(tab_2);
    gridLayout_2->setSpacing(2);
    gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
    gridLayout_2->setContentsMargins(6, 6, 6, 6);
    lbl_FS_Status = new QLabel(tab_2);
    lbl_FS_Status->setObjectName(QString::fromUtf8("lbl_FS_Status"));

    gridLayout_2->addWidget(lbl_FS_Status, 4, 0, 1, 2);

    label_2 = new QLabel(tab_2);
    label_2->setObjectName(QString::fromUtf8("label_2"));

    gridLayout_2->addWidget(label_2, 0, 0, 1, 1);

    progress_FS_Complete = new QProgressBar(tab_2);
    progress_FS_Complete->setObjectName(QString::fromUtf8("progress_FS_Complete"));
    progress_FS_Complete->setValue(0);

    gridLayout_2->addWidget(progress_FS_Complete, 3, 0, 1, 3);

    edit_FS_Log = new QPlainTextEdit(tab_2);
    edit_FS_Log->setObjectName(QString::fromUtf8("edit_FS_Log"));
    edit_FS_Log->setUndoRedoEnabled(false);
    edit_FS_Log->setReadOnly(true);

    gridLayout_2->addWidget(edit_FS_Log, 7, 0, 1, 3);

    edit_FS_Remote = new QLineEdit(tab_2);
    edit_FS_Remote->setObjectName(QString::fromUtf8("edit_FS_Remote"));

    gridLayout_2->addWidget(edit_FS_Remote, 1, 1, 1, 2);

    btn_FS_Local = new QToolButton(tab_2);
    btn_FS_Local->setObjectName(QString::fromUtf8("btn_FS_Local"));

    gridLayout_2->addWidget(btn_FS_Local, 0, 2, 1, 1);

    edit_FS_Local = new QLineEdit(tab_2);
    edit_FS_Local->setObjectName(QString::fromUtf8("edit_FS_Local"));

    gridLayout_2->addWidget(edit_FS_Local, 0, 1, 1, 1);

    label_3 = new QLabel(tab_2);
    label_3->setObjectName(QString::fromUtf8("label_3"));

    gridLayout_2->addWidget(label_3, 1, 0, 1, 1);

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(2);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    radio_FS_Upload = new QRadioButton(tab_2);
    radio_FS_Upload->setObjectName(QString::fromUtf8("radio_FS_Upload"));
    radio_FS_Upload->setChecked(true);

    horizontalLayout->addWidget(radio_FS_Upload);

    radio_FS_Download = new QRadioButton(tab_2);
    radio_FS_Download->setObjectName(QString::fromUtf8("radio_FS_Download"));

    horizontalLayout->addWidget(radio_FS_Download);

    radio_FS_Size = new QRadioButton(tab_2);
    radio_FS_Size->setObjectName(QString::fromUtf8("radio_FS_Size"));

    horizontalLayout->addWidget(radio_FS_Size);

    radio_FS_HashChecksum = new QRadioButton(tab_2);
    radio_FS_HashChecksum->setObjectName(QString::fromUtf8("radio_FS_HashChecksum"));

    horizontalLayout->addWidget(radio_FS_HashChecksum);


    gridLayout_2->addLayout(horizontalLayout, 2, 0, 1, 3);

    horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setSpacing(2);
    horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
    horizontalLayout_2->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_2->addItem(horizontalSpacer);

    btn_FS_Go = new QPushButton(tab_2);
    btn_FS_Go->setObjectName(QString::fromUtf8("btn_FS_Go"));

    horizontalLayout_2->addWidget(btn_FS_Go);

    horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_2->addItem(horizontalSpacer_2);


    gridLayout_2->addLayout(horizontalLayout_2, 5, 0, 1, 3);

    tabWidget_2->addTab(tab_2, QString());
    tab_IMG_Images_2 = new QWidget();
    tab_IMG_Images_2->setObjectName(QString::fromUtf8("tab_IMG_Images_2"));
    gridLayout_3 = new QGridLayout(tab_IMG_Images_2);
    gridLayout_3->setSpacing(2);
    gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
    gridLayout_3->setContentsMargins(6, 6, 6, 6);
    edit_IMG_Log = new QPlainTextEdit(tab_IMG_Images_2);
    edit_IMG_Log->setObjectName(QString::fromUtf8("edit_IMG_Log"));
    edit_IMG_Log->setUndoRedoEnabled(false);
    edit_IMG_Log->setReadOnly(true);

    gridLayout_3->addWidget(edit_IMG_Log, 7, 0, 1, 3);

    horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setSpacing(2);
    horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
    horizontalLayout_3->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_3->addItem(horizontalSpacer_3);

    btn_IMG_Go = new QPushButton(tab_IMG_Images_2);
    btn_IMG_Go->setObjectName(QString::fromUtf8("btn_IMG_Go"));

    horizontalLayout_3->addWidget(btn_IMG_Go);

    horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_3->addItem(horizontalSpacer_4);


    gridLayout_3->addLayout(horizontalLayout_3, 5, 0, 1, 3);

    lbl_IMG_Status = new QLabel(tab_IMG_Images_2);
    lbl_IMG_Status->setObjectName(QString::fromUtf8("lbl_IMG_Status"));

    gridLayout_3->addWidget(lbl_IMG_Status, 3, 0, 1, 2);

    tabWidget_3 = new QTabWidget(tab_IMG_Images_2);
    tabWidget_3->setObjectName(QString::fromUtf8("tabWidget_3"));
    tab_IMG_Upload = new QWidget();
    tab_IMG_Upload->setObjectName(QString::fromUtf8("tab_IMG_Upload"));
    gridLayout_4 = new QGridLayout(tab_IMG_Upload);
    gridLayout_4->setSpacing(2);
    gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
    gridLayout_4->setContentsMargins(6, 6, 6, 6);
    progress_IMG_Complete = new QProgressBar(tab_IMG_Upload);
    progress_IMG_Complete->setObjectName(QString::fromUtf8("progress_IMG_Complete"));
    progress_IMG_Complete->setValue(0);

    gridLayout_4->addWidget(progress_IMG_Complete, 3, 1, 1, 1);

    label_6 = new QLabel(tab_IMG_Upload);
    label_6->setObjectName(QString::fromUtf8("label_6"));

    gridLayout_4->addWidget(label_6, 3, 0, 1, 1);

    horizontalLayout_5 = new QHBoxLayout();
    horizontalLayout_5->setSpacing(2);
    horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
    edit_IMG_Local = new QLineEdit(tab_IMG_Upload);
    edit_IMG_Local->setObjectName(QString::fromUtf8("edit_IMG_Local"));

    horizontalLayout_5->addWidget(edit_IMG_Local);

    btn_IMG_Local = new QToolButton(tab_IMG_Upload);
    btn_IMG_Local->setObjectName(QString::fromUtf8("btn_IMG_Local"));

    horizontalLayout_5->addWidget(btn_IMG_Local);


    gridLayout_4->addLayout(horizontalLayout_5, 0, 1, 1, 1);

    label_4 = new QLabel(tab_IMG_Upload);
    label_4->setObjectName(QString::fromUtf8("label_4"));

    gridLayout_4->addWidget(label_4, 0, 0, 1, 1);

    horizontalLayout_4 = new QHBoxLayout();
    horizontalLayout_4->setSpacing(2);
    horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
    edit_IMG_Image = new QSpinBox(tab_IMG_Upload);
    edit_IMG_Image->setObjectName(QString::fromUtf8("edit_IMG_Image"));
    edit_IMG_Image->setMaximumSize(QSize(60, 16777215));

    horizontalLayout_4->addWidget(edit_IMG_Image);

    radio_IMG_No_Action = new QRadioButton(tab_IMG_Upload);
    radio_IMG_No_Action->setObjectName(QString::fromUtf8("radio_IMG_No_Action"));

    horizontalLayout_4->addWidget(radio_IMG_No_Action);

    radio_IMG_Test = new QRadioButton(tab_IMG_Upload);
    radio_IMG_Test->setObjectName(QString::fromUtf8("radio_IMG_Test"));
    radio_IMG_Test->setChecked(true);

    horizontalLayout_4->addWidget(radio_IMG_Test);

    radio_IMG_Confirm = new QRadioButton(tab_IMG_Upload);
    radio_IMG_Confirm->setObjectName(QString::fromUtf8("radio_IMG_Confirm"));

    horizontalLayout_4->addWidget(radio_IMG_Confirm);


    gridLayout_4->addLayout(horizontalLayout_4, 1, 1, 1, 1);

    label_41 = new QLabel(tab_IMG_Upload);
    label_41->setObjectName(QString::fromUtf8("label_41"));

    gridLayout_4->addWidget(label_41, 1, 0, 1, 1);

    label_9 = new QLabel(tab_IMG_Upload);
    label_9->setObjectName(QString::fromUtf8("label_9"));

    gridLayout_4->addWidget(label_9, 2, 0, 1, 1);

    check_IMG_Reset = new QCheckBox(tab_IMG_Upload);
    check_IMG_Reset->setObjectName(QString::fromUtf8("check_IMG_Reset"));

    gridLayout_4->addWidget(check_IMG_Reset, 2, 1, 1, 1);

    tabWidget_3->addTab(tab_IMG_Upload, QString());
    tab_IMG_Images = new QWidget();
    tab_IMG_Images->setObjectName(QString::fromUtf8("tab_IMG_Images"));
    gridLayout_5 = new QGridLayout(tab_IMG_Images);
    gridLayout_5->setSpacing(2);
    gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
    gridLayout_5->setContentsMargins(6, 6, 6, 6);
    colview_IMG_Images = new QColumnView(tab_IMG_Images);
    colview_IMG_Images->setObjectName(QString::fromUtf8("colview_IMG_Images"));

    gridLayout_5->addWidget(colview_IMG_Images, 0, 0, 1, 1);

    horizontalLayout_6 = new QHBoxLayout();
    horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
    label_5 = new QLabel(tab_IMG_Images);
    label_5->setObjectName(QString::fromUtf8("label_5"));

    horizontalLayout_6->addWidget(label_5);

    radio_IMG_Get = new QRadioButton(tab_IMG_Images);
    radio_IMG_Get->setObjectName(QString::fromUtf8("radio_IMG_Get"));
    radio_IMG_Get->setChecked(true);

    horizontalLayout_6->addWidget(radio_IMG_Get);

    radio_ING_Set = new QRadioButton(tab_IMG_Images);
    radio_ING_Set->setObjectName(QString::fromUtf8("radio_ING_Set"));

    horizontalLayout_6->addWidget(radio_ING_Set);

    horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_6->addItem(horizontalSpacer_5);


    gridLayout_5->addLayout(horizontalLayout_6, 1, 0, 1, 1);

    tabWidget_3->addTab(tab_IMG_Images, QString());
    tab_IMG_Erase = new QWidget();
    tab_IMG_Erase->setObjectName(QString::fromUtf8("tab_IMG_Erase"));
    gridLayout_10 = new QGridLayout(tab_IMG_Erase);
    gridLayout_10->setObjectName(QString::fromUtf8("gridLayout_10"));
    label_14 = new QLabel(tab_IMG_Erase);
    label_14->setObjectName(QString::fromUtf8("label_14"));

    gridLayout_10->addWidget(label_14, 0, 0, 1, 1);

    horizontalSpacer_9 = new QSpacerItem(235, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    gridLayout_10->addItem(horizontalSpacer_9, 0, 2, 1, 1);

    edit_IMG_Erase_Slot = new QSpinBox(tab_IMG_Erase);
    edit_IMG_Erase_Slot->setObjectName(QString::fromUtf8("edit_IMG_Erase_Slot"));
    edit_IMG_Erase_Slot->setMinimumSize(QSize(40, 0));
    edit_IMG_Erase_Slot->setMaximumSize(QSize(16777215, 16777215));

    gridLayout_10->addWidget(edit_IMG_Erase_Slot, 0, 1, 1, 1);

    verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    gridLayout_10->addItem(verticalSpacer_2, 1, 0, 1, 1);

    tabWidget_3->addTab(tab_IMG_Erase, QString());

    gridLayout_3->addWidget(tabWidget_3, 0, 0, 1, 1);

    tabWidget_2->addTab(tab_IMG_Images_2, QString());
    tab_OS = new QWidget();
    tab_OS->setObjectName(QString::fromUtf8("tab_OS"));
    gridLayout_7 = new QGridLayout(tab_OS);
    gridLayout_7->setSpacing(2);
    gridLayout_7->setObjectName(QString::fromUtf8("gridLayout_7"));
    selector_OS = new QTabWidget(tab_OS);
    selector_OS->setObjectName(QString::fromUtf8("selector_OS"));
    tab_OS_Echo = new QWidget();
    tab_OS_Echo->setObjectName(QString::fromUtf8("tab_OS_Echo"));
    gridLayout_8 = new QGridLayout(tab_OS_Echo);
    gridLayout_8->setObjectName(QString::fromUtf8("gridLayout_8"));
    label_10 = new QLabel(tab_OS_Echo);
    label_10->setObjectName(QString::fromUtf8("label_10"));

    gridLayout_8->addWidget(label_10, 0, 0, 1, 1);

    edit_OS_Echo_Input = new QPlainTextEdit(tab_OS_Echo);
    edit_OS_Echo_Input->setObjectName(QString::fromUtf8("edit_OS_Echo_Input"));

    gridLayout_8->addWidget(edit_OS_Echo_Input, 0, 1, 1, 1);

    label_11 = new QLabel(tab_OS_Echo);
    label_11->setObjectName(QString::fromUtf8("label_11"));

    gridLayout_8->addWidget(label_11, 1, 0, 1, 1);

    edit_OS_Echo_Output = new QPlainTextEdit(tab_OS_Echo);
    edit_OS_Echo_Output->setObjectName(QString::fromUtf8("edit_OS_Echo_Output"));
    edit_OS_Echo_Output->setUndoRedoEnabled(false);
    edit_OS_Echo_Output->setReadOnly(true);

    gridLayout_8->addWidget(edit_OS_Echo_Output, 1, 1, 1, 1);

    selector_OS->addTab(tab_OS_Echo, QString());
    tab_OS_Tasks = new QWidget();
    tab_OS_Tasks->setObjectName(QString::fromUtf8("tab_OS_Tasks"));
    selector_OS->addTab(tab_OS_Tasks, QString());
    tab_OS_Memory = new QWidget();
    tab_OS_Memory->setObjectName(QString::fromUtf8("tab_OS_Memory"));
    selector_OS->addTab(tab_OS_Memory, QString());
    tab_OS_Reset = new QWidget();
    tab_OS_Reset->setObjectName(QString::fromUtf8("tab_OS_Reset"));
    gridLayout_12 = new QGridLayout(tab_OS_Reset);
    gridLayout_12->setObjectName(QString::fromUtf8("gridLayout_12"));
    check_OS_Force_Reboot = new QCheckBox(tab_OS_Reset);
    check_OS_Force_Reboot->setObjectName(QString::fromUtf8("check_OS_Force_Reboot"));

    gridLayout_12->addWidget(check_OS_Force_Reboot, 0, 0, 1, 1);

    verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    gridLayout_12->addItem(verticalSpacer_3, 1, 0, 1, 1);

    selector_OS->addTab(tab_OS_Reset, QString());
    tab_OS_Info = new QWidget();
    tab_OS_Info->setObjectName(QString::fromUtf8("tab_OS_Info"));
    gridLayout_13 = new QGridLayout(tab_OS_Info);
    gridLayout_13->setSpacing(2);
    gridLayout_13->setObjectName(QString::fromUtf8("gridLayout_13"));
    label_17 = new QLabel(tab_OS_Info);
    label_17->setObjectName(QString::fromUtf8("label_17"));

    gridLayout_13->addWidget(label_17, 0, 0, 1, 1);

    edit_OS_UName = new QLineEdit(tab_OS_Info);
    edit_OS_UName->setObjectName(QString::fromUtf8("edit_OS_UName"));

    gridLayout_13->addWidget(edit_OS_UName, 0, 1, 1, 1);

    horizontalLayout_10 = new QHBoxLayout();
    horizontalLayout_10->setSpacing(2);
    horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
    radio_FS_Upload_2 = new QRadioButton(tab_OS_Info);
    radio_FS_Upload_2->setObjectName(QString::fromUtf8("radio_FS_Upload_2"));
    radio_FS_Upload_2->setChecked(true);

    horizontalLayout_10->addWidget(radio_FS_Upload_2);

    radio_FS_Download_2 = new QRadioButton(tab_OS_Info);
    radio_FS_Download_2->setObjectName(QString::fromUtf8("radio_FS_Download_2"));

    horizontalLayout_10->addWidget(radio_FS_Download_2);


    gridLayout_13->addLayout(horizontalLayout_10, 1, 0, 1, 2);

    label_18 = new QLabel(tab_OS_Info);
    label_18->setObjectName(QString::fromUtf8("label_18"));

    gridLayout_13->addWidget(label_18, 2, 0, 1, 1);

    edit_OS_Info_Output = new QPlainTextEdit(tab_OS_Info);
    edit_OS_Info_Output->setObjectName(QString::fromUtf8("edit_OS_Info_Output"));

    gridLayout_13->addWidget(edit_OS_Info_Output, 2, 1, 1, 1);

    selector_OS->addTab(tab_OS_Info, QString());

    gridLayout_7->addWidget(selector_OS, 0, 0, 1, 1);

    horizontalLayout_13 = new QHBoxLayout();
    horizontalLayout_13->setSpacing(2);
    horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
    horizontalLayout_13->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_17 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_13->addItem(horizontalSpacer_17);

    btn_OS_Go = new QPushButton(tab_OS);
    btn_OS_Go->setObjectName(QString::fromUtf8("btn_OS_Go"));

    horizontalLayout_13->addWidget(btn_OS_Go);

    horizontalSpacer_18 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_13->addItem(horizontalSpacer_18);


    gridLayout_7->addLayout(horizontalLayout_13, 2, 0, 1, 1);

    lbl_OS_Status = new QLabel(tab_OS);
    lbl_OS_Status->setObjectName(QString::fromUtf8("lbl_OS_Status"));

    gridLayout_7->addWidget(lbl_OS_Status, 1, 0, 1, 1);

    tabWidget_2->addTab(tab_OS, QString());
    tab_5 = new QWidget();
    tab_5->setObjectName(QString::fromUtf8("tab_5"));
    gridLayout_11 = new QGridLayout(tab_5);
    gridLayout_11->setObjectName(QString::fromUtf8("gridLayout_11"));
    label_15 = new QLabel(tab_5);
    label_15->setObjectName(QString::fromUtf8("label_15"));

    gridLayout_11->addWidget(label_15, 0, 0, 1, 1);

    combo_STAT_Group = new QComboBox(tab_5);
    combo_STAT_Group->setObjectName(QString::fromUtf8("combo_STAT_Group"));
    combo_STAT_Group->setEditable(true);

    gridLayout_11->addWidget(combo_STAT_Group, 0, 1, 1, 1);

    label_16 = new QLabel(tab_5);
    label_16->setObjectName(QString::fromUtf8("label_16"));

    gridLayout_11->addWidget(label_16, 1, 0, 1, 1);

    table_STAT_Values = new QTableWidget(tab_5);
    table_STAT_Values->setObjectName(QString::fromUtf8("table_STAT_Values"));

    gridLayout_11->addWidget(table_STAT_Values, 1, 1, 1, 1);

    horizontalLayout_9 = new QHBoxLayout();
    horizontalLayout_9->setSpacing(2);
    horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
    radio_STAT_List = new QRadioButton(tab_5);
    radio_STAT_List->setObjectName(QString::fromUtf8("radio_STAT_List"));
    radio_STAT_List->setChecked(true);

    horizontalLayout_9->addWidget(radio_STAT_List);

    radio_STAT_Fetch = new QRadioButton(tab_5);
    radio_STAT_Fetch->setObjectName(QString::fromUtf8("radio_STAT_Fetch"));

    horizontalLayout_9->addWidget(radio_STAT_Fetch);


    gridLayout_11->addLayout(horizontalLayout_9, 2, 0, 1, 2);

    lbl_STAT_Status = new QLabel(tab_5);
    lbl_STAT_Status->setObjectName(QString::fromUtf8("lbl_STAT_Status"));

    gridLayout_11->addWidget(lbl_STAT_Status, 3, 0, 1, 2);

    horizontalLayout_14 = new QHBoxLayout();
    horizontalLayout_14->setSpacing(2);
    horizontalLayout_14->setObjectName(QString::fromUtf8("horizontalLayout_14"));
    horizontalLayout_14->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_19 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_14->addItem(horizontalSpacer_19);

    btn_STAT_Go = new QPushButton(tab_5);
    btn_STAT_Go->setObjectName(QString::fromUtf8("btn_STAT_Go"));

    horizontalLayout_14->addWidget(btn_STAT_Go);

    horizontalSpacer_20 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_14->addItem(horizontalSpacer_20);


    gridLayout_11->addLayout(horizontalLayout_14, 4, 0, 1, 2);

    tabWidget_2->addTab(tab_5, QString());
    tab_6 = new QWidget();
    tab_6->setObjectName(QString::fromUtf8("tab_6"));
    gridLayout_9 = new QGridLayout(tab_6);
    gridLayout_9->setSpacing(2);
    gridLayout_9->setObjectName(QString::fromUtf8("gridLayout_9"));
    verticalLayout_3 = new QVBoxLayout();
    verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
    btn_SHELL_Clear = new QToolButton(tab_6);
    btn_SHELL_Clear->setObjectName(QString::fromUtf8("btn_SHELL_Clear"));

    verticalLayout_3->addWidget(btn_SHELL_Clear);

    btn_SHELL_Copy = new QToolButton(tab_6);
    btn_SHELL_Copy->setObjectName(QString::fromUtf8("btn_SHELL_Copy"));

    verticalLayout_3->addWidget(btn_SHELL_Copy);


    gridLayout_9->addLayout(verticalLayout_3, 1, 2, 1, 1);

    lbl_SHELL_Status = new QLabel(tab_6);
    lbl_SHELL_Status->setObjectName(QString::fromUtf8("lbl_SHELL_Status"));

    gridLayout_9->addWidget(lbl_SHELL_Status, 2, 0, 1, 3);

    edit_SHELL_Output = new QPlainTextEdit(tab_6);
    edit_SHELL_Output->setObjectName(QString::fromUtf8("edit_SHELL_Output"));
    edit_SHELL_Output->setUndoRedoEnabled(false);
    edit_SHELL_Output->setReadOnly(true);

    gridLayout_9->addWidget(edit_SHELL_Output, 1, 1, 1, 1);

    edit_SHELL_Input = new QLineEdit(tab_6);
    edit_SHELL_Input->setObjectName(QString::fromUtf8("edit_SHELL_Input"));

    gridLayout_9->addWidget(edit_SHELL_Input, 0, 1, 1, 2);

    label_12 = new QLabel(tab_6);
    label_12->setObjectName(QString::fromUtf8("label_12"));

    gridLayout_9->addWidget(label_12, 0, 0, 1, 1);

    horizontalLayout_8 = new QHBoxLayout();
    horizontalLayout_8->setSpacing(2);
    horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
    horizontalLayout_8->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_8->addItem(horizontalSpacer_7);

    btn_SHELL_Go = new QPushButton(tab_6);
    btn_SHELL_Go->setObjectName(QString::fromUtf8("btn_SHELL_Go"));

    horizontalLayout_8->addWidget(btn_SHELL_Go);

    horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_8->addItem(horizontalSpacer_8);


    gridLayout_9->addLayout(horizontalLayout_8, 3, 0, 1, 3);

    label_13 = new QLabel(tab_6);
    label_13->setObjectName(QString::fromUtf8("label_13"));

    gridLayout_9->addWidget(label_13, 1, 0, 1, 1);

    tabWidget_2->addTab(tab_6, QString());
//    verticalLayoutWidget = new QWidget(tab);
    verticalLayoutWidget = new QWidget();
    verticalLayoutWidget->setObjectName(QString::fromUtf8("verticalLayoutWidget"));
//    verticalLayoutWidget->setGeometry(QRect(410, 20, 229, 182));
    verticalLayoutWidget->setGeometry(QRect(6, 6, 229, 182));
    verticalLayout = new QVBoxLayout(verticalLayoutWidget);
    verticalLayout->setSpacing(2);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    formLayout = new QFormLayout();
    formLayout->setObjectName(QString::fromUtf8("formLayout"));
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setHorizontalSpacing(2);
    formLayout->setVerticalSpacing(2);
    label_7 = new QLabel(verticalLayoutWidget);
    label_7->setObjectName(QString::fromUtf8("label_7"));

    formLayout->setWidget(0, QFormLayout::LabelRole, label_7);

    edit_IMG_Preview_Hash = new QLineEdit(verticalLayoutWidget);
    edit_IMG_Preview_Hash->setObjectName(QString::fromUtf8("edit_IMG_Preview_Hash"));
    edit_IMG_Preview_Hash->setReadOnly(true);

    formLayout->setWidget(0, QFormLayout::FieldRole, edit_IMG_Preview_Hash);

    label_8 = new QLabel(verticalLayoutWidget);
    label_8->setObjectName(QString::fromUtf8("label_8"));

    formLayout->setWidget(1, QFormLayout::LabelRole, label_8);

    edit_IMG_Preview_Version = new QLineEdit(verticalLayoutWidget);
    edit_IMG_Preview_Version->setObjectName(QString::fromUtf8("edit_IMG_Preview_Version"));
    edit_IMG_Preview_Version->setReadOnly(true);

    formLayout->setWidget(1, QFormLayout::FieldRole, edit_IMG_Preview_Version);


    verticalLayout->addLayout(formLayout);

    gridLayout_6 = new QGridLayout();
    gridLayout_6->setSpacing(2);
    gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
    check_IMG_Preview_Confirmed = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Confirmed->setObjectName(QString::fromUtf8("check_IMG_Preview_Confirmed"));

    gridLayout_6->addWidget(check_IMG_Preview_Confirmed, 1, 0, 1, 1);

    check_IMG_Preview_Active = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Active->setObjectName(QString::fromUtf8("check_IMG_Preview_Active"));
    check_IMG_Preview_Active->setEnabled(true);
    check_IMG_Preview_Active->setCheckable(true);

    gridLayout_6->addWidget(check_IMG_Preview_Active, 0, 0, 1, 1);

    check_IMG_Preview_Pending = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Pending->setObjectName(QString::fromUtf8("check_IMG_Preview_Pending"));

    gridLayout_6->addWidget(check_IMG_Preview_Pending, 1, 1, 1, 1);

    check_IMG_Preview_Bootable = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Bootable->setObjectName(QString::fromUtf8("check_IMG_Preview_Bootable"));

    gridLayout_6->addWidget(check_IMG_Preview_Bootable, 0, 1, 1, 1);

    check_IMG_Preview_Permanent = new QCheckBox(verticalLayoutWidget);
    check_IMG_Preview_Permanent->setObjectName(QString::fromUtf8("check_IMG_Preview_Permanent"));

    gridLayout_6->addWidget(check_IMG_Preview_Permanent, 2, 0, 1, 1);


    verticalLayout->addLayout(gridLayout_6);

    btn_IMG_Preview_Copy = new QPushButton(verticalLayoutWidget);
    btn_IMG_Preview_Copy->setObjectName(QString::fromUtf8("btn_IMG_Preview_Copy"));

    verticalLayout->addWidget(btn_IMG_Preview_Copy);

    verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout->addItem(verticalSpacer);

    horizontalLayoutWidget = new QWidget(tab);
    horizontalLayoutWidget->setObjectName(QString::fromUtf8("horizontalLayoutWidget"));
    horizontalLayoutWidget->setGeometry(QRect(10, 10, 341, 31));
    horizontalLayout_7 = new QHBoxLayout(horizontalLayoutWidget);
    horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
    horizontalLayout_7->setContentsMargins(0, 0, 0, 0);
    label = new QLabel(horizontalLayoutWidget);
    label->setObjectName(QString::fromUtf8("label"));

    horizontalLayout_7->addWidget(label);

    edit_MTU = new QSpinBox(horizontalLayoutWidget);
    edit_MTU->setObjectName(QString::fromUtf8("edit_MTU"));
    edit_MTU->setMinimum(32);
    edit_MTU->setMaximum(2048);
    edit_MTU->setValue(128);

    horizontalLayout_7->addWidget(edit_MTU);

    check_V2_Protocol = new QCheckBox(horizontalLayoutWidget);
    check_V2_Protocol->setObjectName(QString::fromUtf8("check_V2_Protocol"));
    check_V2_Protocol->setChecked(true);

    horizontalLayout_7->addWidget(check_V2_Protocol);

    horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_7->addItem(horizontalSpacer_6);

//    tabWidget->addTab(tab, QString());

//    gridLayout->addWidget(tabWidget, 0, 0, 1, 1);


//    retranslateUi(Form);

//    tabWidget->setCurrentIndex(0);
    tabWidget_2->setCurrentIndex(1);
    tabWidget_3->setCurrentIndex(0);
    selector_OS->setCurrentIndex(2);

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
    check_IMG_Reset->setText(QCoreApplication::translate("Form", "After upload", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Upload), QCoreApplication::translate("Form", "Upload", nullptr));
    label_5->setText(QCoreApplication::translate("Form", "State:", nullptr));
    radio_IMG_Get->setText(QCoreApplication::translate("Form", "Get", nullptr));
    radio_ING_Set->setText(QCoreApplication::translate("Form", "Set", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Images), QCoreApplication::translate("Form", "Images", nullptr));
    label_14->setText(QCoreApplication::translate("Form", "Slot:", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Erase), QCoreApplication::translate("Form", "Erase", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_IMG_Images_2), QCoreApplication::translate("Form", "Img", nullptr));
    label_10->setText(QCoreApplication::translate("Form", "Input:", nullptr));
    label_11->setText(QCoreApplication::translate("Form", "Output:", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Echo), QCoreApplication::translate("Form", "Echo", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Tasks), QCoreApplication::translate("Form", "Stats", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Memory), QCoreApplication::translate("Form", "Memory", nullptr));
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
    btn_SHELL_Clear->setText(QCoreApplication::translate("Form", "Clear", nullptr));
    btn_SHELL_Copy->setText(QCoreApplication::translate("Form", "Copy", nullptr));
    lbl_SHELL_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    label_12->setText(QCoreApplication::translate("Form", "Input:", nullptr));
    btn_SHELL_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    label_13->setText(QCoreApplication::translate("Form", "Output:", nullptr));
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
    my_img = new smp_group_img_mgmt(processor);
    my_os = new smp_group_os_mgmt(processor);

    connect(my_img, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
    connect(my_img, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
    connect(my_img, SIGNAL(plugin_to_hex(QByteArray*)), this, SLOT(group_to_hex(QByteArray*)));

    connect(my_os, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
    connect(my_os, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
//    connect(my_os, SIGNAL(plugin_to_hex(QByteArray*)), this, SLOT(group_to_hex(QByteArray*)));
}

plugin_mcumgr::~plugin_mcumgr()
{
    delete my_os;
    delete my_img;
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

void plugin_mcumgr::receive_waiting(QByteArray message)
{
#if 0
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
#endif
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
    if (tabWidget_3->currentWidget() == tab_IMG_Upload)
    {
        //Upload
        emit plugin_set_status(true, false);

        my_img->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_IMG_UPLOAD);
        my_img->start_firmware_update(edit_IMG_Image->value(), edit_IMG_Local->text(), false, &upload_hash);

        progress_IMG_Complete->setValue(0);
        lbl_IMG_Status->setText("Uploading...");
    }
    else if (tabWidget_3->currentWidget() == tab_IMG_Images)
    {
        //Image list
        emit plugin_set_status(true, false);

        colview_IMG_Images->previewWidget()->hide();
        model_image_state.clear();
        blaharray.clear();
        my_img->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_IMG_IMAGE_LIST);
        my_img->start_image_get(&blaharray);

        progress_IMG_Complete->setValue(0);
        lbl_IMG_Status->setText("Querying...");
    }
    else if (tabWidget_3->currentWidget() == tab_IMG_Erase)
    {
        //Erase
        emit plugin_set_status(true, false);
        my_img->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_erase_ms, ACTION_IMG_IMAGE_ERASE);
        my_img->start_image_erase(edit_IMG_Erase_Slot->value());
        progress_IMG_Complete->setValue(0);
        lbl_IMG_Status->setText("Erasing...");
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
        emit plugin_set_status(true, false);

        edit_OS_Echo_Output->clear();
        my_os->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_OS_ECHO);
        my_os->start_echo(edit_OS_Echo_Input->toPlainText());

        lbl_OS_Status->setText("Echoing...");
    }
    else if (selector_OS->currentWidget() == tab_OS_Tasks)
    {
        emit plugin_set_status(true, false);

        my_os->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_OS_TASK_STATS);
        my_os->start_task_stats();

        edit_OS_Echo_Output->appendPlainText("Tasking...");
    }
    else if (selector_OS->currentWidget() == tab_OS_Memory)
    {
        emit plugin_set_status(true, false);

        my_os->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_OS_MEMORY_POOL);
        my_os->start_memory_pool();

        edit_OS_Echo_Output->appendPlainText("Memorying...");
    }
    else if (selector_OS->currentWidget() == tab_OS_Reset)
    {
        emit plugin_set_status(true, false);

        my_os->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_OS_RESET);
        my_os->start_reset(check_OS_Force_Reboot->isChecked());

        edit_OS_Echo_Output->appendPlainText("Resetting...");
    }
    else if (selector_OS->currentWidget() == tab_OS_Info)
    {
        if (radio_FS_Download_2->isChecked())
        {
            //uname
            emit plugin_set_status(true, false);

            my_os->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_OS_OS_APPLICATION_INFO);
            my_os->start_os_application_info(edit_OS_UName->text());

            edit_OS_Echo_Output->appendPlainText("Infoing...");
        }
        else
        {
            //Buffer details
            emit plugin_set_status(true, false);

            my_os->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_OS_MCUMGR_BUFFER);
            my_os->start_mcumgr_parameters();

            edit_OS_Echo_Output->appendPlainText("Buffering...");
        }
    }
}

void plugin_mcumgr::on_btn_SHELL_Go_clicked()
{
#if 0
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
        processor->send(tmp_message, timeout_ms, 3, true);

        lbl_SHELL_Status->setText("Executing...");
#endif
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

void plugin_mcumgr::status(uint8_t user_data, group_status status, QString error_string)
{
    /*
     *     STATUS_COMPLETE = 0,
    STATUS_ERROR,
    STATUS_TIMEOUT,
    STATUS_CANCELLED
*/

    bool finished = true;

    qDebug() << "Status: " << status << "Sender: " << sender();
    if (sender() == my_img)
    {
        qDebug() << "img sender";
        if (status == STATUS_COMPLETE)
        {
            qDebug() << "complete";
            //Advance to next stage of image upload
            if (user_data == ACTION_IMG_UPLOAD)
            {
                qDebug() << "is upload";
                if (radio_IMG_Test->isChecked() || radio_IMG_Confirm->isChecked())
                {
                    //Mark image for test or confirmation
                    finished = false;

                    my_img->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_IMG_UPLOAD_SET);
                    my_img->start_image_set(&upload_hash, (radio_IMG_Confirm->isChecked() ? true : false));
                    qDebug() << "do upload of " << upload_hash;
                }
                else
                {
                    edit_IMG_Log->appendPlainText("Finished");
                }
            }
            else if (user_data == ACTION_IMG_UPLOAD_SET)
            {
                if (check_IMG_Reset->isChecked())
                {
                    //Reboot device
                    finished = false;

                    my_os->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, ACTION_OS_UPLOAD_RESET);
                    my_os->start_reset(false);
                    qDebug() << "do reset";
                }
                else
                {
                    edit_IMG_Log->appendPlainText("Finished and marked as test/confirm");
                }
            }
            else if (user_data == ACTION_IMG_IMAGE_LIST)
            {
                uint8_t i = 0;
                while (i < blaharray.length())
                {
                    model_image_state.appendRow(blaharray[i].item);
                    ++i;
                }
            }
        }
    }
    else if (sender() == my_os)
    {
        qDebug() << "os sender";
        if (status == STATUS_COMPLETE)
        {
            qDebug() << "complete";
            if (user_data == ACTION_OS_ECHO)
            {
                edit_OS_Echo_Output->appendPlainText(error_string);
            }
            else if (user_data == ACTION_OS_UPLOAD_RESET)
            {
                edit_IMG_Log->appendPlainText("Finished and reset");
            }
        }
    }

    if (finished == true)
    {
        emit plugin_set_status(false, false);
    }

    if (error_string != nullptr)
    {
        qDebug() << "Status message: " << error_string;
    }
}

void plugin_mcumgr::progress(uint8_t user_data, uint8_t percent)
{
    progress_IMG_Complete->setValue(percent);
}

void plugin_mcumgr::group_to_hex(QByteArray *data)
{
    emit plugin_to_hex(data);
}
