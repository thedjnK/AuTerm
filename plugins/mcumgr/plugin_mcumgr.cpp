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
#include <QDebug>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QRegularExpression>
#include <QClipboard>
#include "plugin_mcumgr.h"

const uint8_t retries = 3;
const uint16_t timeout_ms = 3000;
const uint16_t timeout_erase_ms = 14000;

static QMainWindow *parent_window;

void plugin_mcumgr::setup(QMainWindow *main_window)
{
    parent_window = main_window;

    uart_transport = new smp_uart(this);

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    udp_transport = new smp_udp(this);
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    bluetooth_transport = new smp_bluetooth(this);
#endif

    processor = new smp_processor(this);
    mode = ACTION_IDLE;

    parent_row = -1;
    parent_column = -1;
    child_row = -1;
    child_column = -1;

    QTabWidget *tabWidget_orig = parent_window->findChild<QTabWidget *>("selector_Tab");
//    QPushButton *other = main_window->findChild<QPushButton *>("btn_TermClose");

///AUTOGEN_START_INIT
//    gridLayout = new QGridLayout(Form);
//    gridLayout->setObjectName("gridLayout");
//    tabWidget = new QTabWidget(Form);
//    tabWidget->setObjectName("tabWidget");
    tab = new QWidget(tabWidget_orig);
    tab->setObjectName("tab");
    verticalLayout_2 = new QVBoxLayout(tab);
    verticalLayout_2->setObjectName("verticalLayout_2");
    horizontalLayout_7 = new QHBoxLayout();
    horizontalLayout_7->setObjectName("horizontalLayout_7");
    label = new QLabel(tab);
    label->setObjectName("label");

    horizontalLayout_7->addWidget(label);

    edit_MTU = new QSpinBox(tab);
    edit_MTU->setObjectName("edit_MTU");
    edit_MTU->setMinimum(32);
    edit_MTU->setMaximum(8192);
    edit_MTU->setValue(256);

    horizontalLayout_7->addWidget(edit_MTU);

    check_V2_Protocol = new QCheckBox(tab);
    check_V2_Protocol->setObjectName("check_V2_Protocol");
    check_V2_Protocol->setChecked(true);

    horizontalLayout_7->addWidget(check_V2_Protocol);

    radio_transport_uart = new QRadioButton(tab);
    radio_transport_uart->setObjectName("radio_transport_uart");
    radio_transport_uart->setChecked(true);

    horizontalLayout_7->addWidget(radio_transport_uart);

    radio_transport_udp = new QRadioButton(tab);
    radio_transport_udp->setObjectName("radio_transport_udp");

    horizontalLayout_7->addWidget(radio_transport_udp);

    radio_transport_bluetooth = new QRadioButton(tab);
    radio_transport_bluetooth->setObjectName("radio_transport_bluetooth");

    horizontalLayout_7->addWidget(radio_transport_bluetooth);

    btn_transport_connect = new QPushButton(tab);
    btn_transport_connect->setObjectName("btn_transport_connect");

    horizontalLayout_7->addWidget(btn_transport_connect);

    horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_7->addItem(horizontalSpacer_6);


    verticalLayout_2->addLayout(horizontalLayout_7);

    tabWidget_2 = new QTabWidget(tab);
    tabWidget_2->setObjectName("tabWidget_2");
    tabWidget_2->setTabPosition(QTabWidget::West);
    tab_FS = new QWidget();
    tab_FS->setObjectName("tab_FS");
    gridLayout_2 = new QGridLayout(tab_FS);
    gridLayout_2->setSpacing(2);
    gridLayout_2->setObjectName("gridLayout_2");
    gridLayout_2->setContentsMargins(6, 6, 6, 6);
    edit_FS_Local = new QLineEdit(tab_FS);
    edit_FS_Local->setObjectName("edit_FS_Local");

    gridLayout_2->addWidget(edit_FS_Local, 0, 1, 1, 1);

    btn_FS_Local = new QToolButton(tab_FS);
    btn_FS_Local->setObjectName("btn_FS_Local");

    gridLayout_2->addWidget(btn_FS_Local, 0, 2, 1, 1);

    horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setSpacing(2);
    horizontalLayout_2->setObjectName("horizontalLayout_2");
    horizontalLayout_2->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_2->addItem(horizontalSpacer);

    btn_FS_Go = new QPushButton(tab_FS);
    btn_FS_Go->setObjectName("btn_FS_Go");

    horizontalLayout_2->addWidget(btn_FS_Go);

    horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_2->addItem(horizontalSpacer_2);


    gridLayout_2->addLayout(horizontalLayout_2, 6, 0, 1, 3);

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(2);
    horizontalLayout->setObjectName("horizontalLayout");
    radio_FS_Upload = new QRadioButton(tab_FS);
    radio_FS_Upload->setObjectName("radio_FS_Upload");
    radio_FS_Upload->setChecked(true);

    horizontalLayout->addWidget(radio_FS_Upload);

    radio_FS_Download = new QRadioButton(tab_FS);
    radio_FS_Download->setObjectName("radio_FS_Download");

    horizontalLayout->addWidget(radio_FS_Download);

    radio_FS_Size = new QRadioButton(tab_FS);
    radio_FS_Size->setObjectName("radio_FS_Size");

    horizontalLayout->addWidget(radio_FS_Size);

    radio_FS_HashChecksum = new QRadioButton(tab_FS);
    radio_FS_HashChecksum->setObjectName("radio_FS_HashChecksum");

    horizontalLayout->addWidget(radio_FS_HashChecksum);

    radio_FS_Hash_Checksum_Types = new QRadioButton(tab_FS);
    radio_FS_Hash_Checksum_Types->setObjectName("radio_FS_Hash_Checksum_Types");

    horizontalLayout->addWidget(radio_FS_Hash_Checksum_Types);


    gridLayout_2->addLayout(horizontalLayout, 3, 0, 1, 3);

    lbl_FS_Status = new QLabel(tab_FS);
    lbl_FS_Status->setObjectName("lbl_FS_Status");

    gridLayout_2->addWidget(lbl_FS_Status, 5, 0, 1, 2);

    label_2 = new QLabel(tab_FS);
    label_2->setObjectName("label_2");

    gridLayout_2->addWidget(label_2, 0, 0, 1, 1);

    edit_FS_Log = new QPlainTextEdit(tab_FS);
    edit_FS_Log->setObjectName("edit_FS_Log");
    edit_FS_Log->setUndoRedoEnabled(false);
    edit_FS_Log->setReadOnly(true);

    gridLayout_2->addWidget(edit_FS_Log, 8, 0, 1, 3);

    label_3 = new QLabel(tab_FS);
    label_3->setObjectName("label_3");

    gridLayout_2->addWidget(label_3, 1, 0, 1, 1);

    progress_FS_Complete = new QProgressBar(tab_FS);
    progress_FS_Complete->setObjectName("progress_FS_Complete");
    progress_FS_Complete->setValue(0);

    gridLayout_2->addWidget(progress_FS_Complete, 4, 0, 1, 3);

    edit_FS_Remote = new QLineEdit(tab_FS);
    edit_FS_Remote->setObjectName("edit_FS_Remote");

    gridLayout_2->addWidget(edit_FS_Remote, 1, 1, 1, 2);

    label_19 = new QLabel(tab_FS);
    label_19->setObjectName("label_19");

    gridLayout_2->addWidget(label_19, 2, 0, 1, 1);

    combo_FS_Hash_Checksum = new QComboBox(tab_FS);
    combo_FS_Hash_Checksum->setObjectName("combo_FS_Hash_Checksum");
    combo_FS_Hash_Checksum->setEditable(true);

    gridLayout_2->addWidget(combo_FS_Hash_Checksum, 2, 1, 1, 2);

    tabWidget_2->addTab(tab_FS, QString());
    tab_IMG = new QWidget();
    tab_IMG->setObjectName("tab_IMG");
    gridLayout_3 = new QGridLayout(tab_IMG);
    gridLayout_3->setSpacing(2);
    gridLayout_3->setObjectName("gridLayout_3");
    gridLayout_3->setContentsMargins(6, 6, 6, 6);
    tabWidget_3 = new QTabWidget(tab_IMG);
    tabWidget_3->setObjectName("tabWidget_3");
    tab_IMG_Upload = new QWidget();
    tab_IMG_Upload->setObjectName("tab_IMG_Upload");
    gridLayout_4 = new QGridLayout(tab_IMG_Upload);
    gridLayout_4->setSpacing(2);
    gridLayout_4->setObjectName("gridLayout_4");
    gridLayout_4->setContentsMargins(6, 6, 6, 6);
    label_4 = new QLabel(tab_IMG_Upload);
    label_4->setObjectName("label_4");

    gridLayout_4->addWidget(label_4, 0, 0, 1, 1);

    check_IMG_Reset = new QCheckBox(tab_IMG_Upload);
    check_IMG_Reset->setObjectName("check_IMG_Reset");

    gridLayout_4->addWidget(check_IMG_Reset, 2, 1, 1, 1);

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

    label_41 = new QLabel(tab_IMG_Upload);
    label_41->setObjectName("label_41");

    gridLayout_4->addWidget(label_41, 1, 0, 1, 1);

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

    progress_IMG_Complete = new QProgressBar(tab_IMG_Upload);
    progress_IMG_Complete->setObjectName("progress_IMG_Complete");
    progress_IMG_Complete->setValue(0);

    gridLayout_4->addWidget(progress_IMG_Complete, 3, 1, 1, 1);

    label_9 = new QLabel(tab_IMG_Upload);
    label_9->setObjectName("label_9");

    gridLayout_4->addWidget(label_9, 2, 0, 1, 1);

    verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    gridLayout_4->addItem(verticalSpacer_4, 4, 0, 1, 1);

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
    horizontalLayout_6->setSpacing(2);
    horizontalLayout_6->setObjectName("horizontalLayout_6");
    label_5 = new QLabel(tab_IMG_Images);
    label_5->setObjectName("label_5");

    horizontalLayout_6->addWidget(label_5);

    radio_IMG_Get = new QRadioButton(tab_IMG_Images);
    radio_IMG_Get->setObjectName("radio_IMG_Get");
    radio_IMG_Get->setChecked(true);

    horizontalLayout_6->addWidget(radio_IMG_Get);

    radio_IMG_Set = new QRadioButton(tab_IMG_Images);
    radio_IMG_Set->setObjectName("radio_IMG_Set");

    horizontalLayout_6->addWidget(radio_IMG_Set);

    line = new QFrame(tab_IMG_Images);
    line->setObjectName("line");
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);

    horizontalLayout_6->addWidget(line);

    check_IMG_Confirm = new QCheckBox(tab_IMG_Images);
    check_IMG_Confirm->setObjectName("check_IMG_Confirm");
    check_IMG_Confirm->setEnabled(false);

    horizontalLayout_6->addWidget(check_IMG_Confirm);

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

    horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setSpacing(2);
    horizontalLayout_3->setObjectName("horizontalLayout_3");
    horizontalLayout_3->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_3->addItem(horizontalSpacer_3);

    btn_IMG_Go = new QPushButton(tab_IMG);
    btn_IMG_Go->setObjectName("btn_IMG_Go");

    horizontalLayout_3->addWidget(btn_IMG_Go);

    horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_3->addItem(horizontalSpacer_4);


    gridLayout_3->addLayout(horizontalLayout_3, 5, 0, 1, 3);

    lbl_IMG_Status = new QLabel(tab_IMG);
    lbl_IMG_Status->setObjectName("lbl_IMG_Status");

    gridLayout_3->addWidget(lbl_IMG_Status, 3, 0, 1, 2);

    tabWidget_2->addTab(tab_IMG, QString());
    tab_OS = new QWidget();
    tab_OS->setObjectName("tab_OS");
    gridLayout_7 = new QGridLayout(tab_OS);
    gridLayout_7->setSpacing(2);
    gridLayout_7->setObjectName("gridLayout_7");
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
    tab_OS_Tasks = new QWidget();
    tab_OS_Tasks->setObjectName("tab_OS_Tasks");
    gridLayout_14 = new QGridLayout(tab_OS_Tasks);
    gridLayout_14->setObjectName("gridLayout_14");
    table_OS_Tasks = new QTableWidget(tab_OS_Tasks);
    if (table_OS_Tasks->columnCount() < 8)
        table_OS_Tasks->setColumnCount(8);
    QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
    table_OS_Tasks->setHorizontalHeaderItem(0, __qtablewidgetitem);
    QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
    table_OS_Tasks->setHorizontalHeaderItem(1, __qtablewidgetitem1);
    QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
    table_OS_Tasks->setHorizontalHeaderItem(2, __qtablewidgetitem2);
    QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
    table_OS_Tasks->setHorizontalHeaderItem(3, __qtablewidgetitem3);
    QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
    table_OS_Tasks->setHorizontalHeaderItem(4, __qtablewidgetitem4);
    QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
    table_OS_Tasks->setHorizontalHeaderItem(5, __qtablewidgetitem5);
    QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
    table_OS_Tasks->setHorizontalHeaderItem(6, __qtablewidgetitem6);
    QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
    table_OS_Tasks->setHorizontalHeaderItem(7, __qtablewidgetitem7);
    table_OS_Tasks->setObjectName("table_OS_Tasks");
    table_OS_Tasks->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_OS_Tasks->setProperty("showDropIndicator", QVariant(false));
    table_OS_Tasks->setDragDropOverwriteMode(false);
    table_OS_Tasks->setAlternatingRowColors(true);
    table_OS_Tasks->setSelectionMode(QAbstractItemView::NoSelection);
    table_OS_Tasks->setSortingEnabled(true);
    table_OS_Tasks->setCornerButtonEnabled(false);

    gridLayout_14->addWidget(table_OS_Tasks, 0, 0, 1, 1);

    selector_OS->addTab(tab_OS_Tasks, QString());
    tab_OS_Memory = new QWidget();
    tab_OS_Memory->setObjectName("tab_OS_Memory");
    verticalLayout_4 = new QVBoxLayout(tab_OS_Memory);
    verticalLayout_4->setObjectName("verticalLayout_4");
    table_OS_Memory = new QTableWidget(tab_OS_Memory);
    if (table_OS_Memory->columnCount() < 4)
        table_OS_Memory->setColumnCount(4);
    QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
    table_OS_Memory->setHorizontalHeaderItem(0, __qtablewidgetitem8);
    QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
    table_OS_Memory->setHorizontalHeaderItem(1, __qtablewidgetitem9);
    QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
    table_OS_Memory->setHorizontalHeaderItem(2, __qtablewidgetitem10);
    QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
    table_OS_Memory->setHorizontalHeaderItem(3, __qtablewidgetitem11);
    table_OS_Memory->setObjectName("table_OS_Memory");
    table_OS_Memory->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_OS_Memory->setProperty("showDropIndicator", QVariant(false));
    table_OS_Memory->setDragDropOverwriteMode(false);
    table_OS_Memory->setAlternatingRowColors(true);
    table_OS_Memory->setSelectionMode(QAbstractItemView::NoSelection);
    table_OS_Memory->setSortingEnabled(true);
    table_OS_Memory->setCornerButtonEnabled(false);

    verticalLayout_4->addWidget(table_OS_Memory);

    selector_OS->addTab(tab_OS_Memory, QString());
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

    edit_OS_UName = new QLineEdit(tab_OS_Info);
    edit_OS_UName->setObjectName("edit_OS_UName");
    edit_OS_UName->setEnabled(false);
    edit_OS_UName->setReadOnly(false);

    gridLayout_13->addWidget(edit_OS_UName, 0, 1, 1, 1);

    horizontalLayout_10 = new QHBoxLayout();
    horizontalLayout_10->setSpacing(2);
    horizontalLayout_10->setObjectName("horizontalLayout_10");
    radio_OS_Buffer_Info = new QRadioButton(tab_OS_Info);
    radio_OS_Buffer_Info->setObjectName("radio_OS_Buffer_Info");
    radio_OS_Buffer_Info->setChecked(true);

    horizontalLayout_10->addWidget(radio_OS_Buffer_Info);

    radio_OS_uname = new QRadioButton(tab_OS_Info);
    radio_OS_uname->setObjectName("radio_OS_uname");

    horizontalLayout_10->addWidget(radio_OS_uname);


    gridLayout_13->addLayout(horizontalLayout_10, 1, 0, 1, 2);

    label_18 = new QLabel(tab_OS_Info);
    label_18->setObjectName("label_18");

    gridLayout_13->addWidget(label_18, 2, 0, 1, 1);

    edit_OS_Info_Output = new QPlainTextEdit(tab_OS_Info);
    edit_OS_Info_Output->setObjectName("edit_OS_Info_Output");

    gridLayout_13->addWidget(edit_OS_Info_Output, 2, 1, 1, 1);

    selector_OS->addTab(tab_OS_Info, QString());
    tab_OS_Bootloader = new QWidget();
    tab_OS_Bootloader->setObjectName("tab_OS_Bootloader");
    formLayout_2 = new QFormLayout(tab_OS_Bootloader);
    formLayout_2->setObjectName("formLayout_2");
    formLayout_2->setFormAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
    formLayout_2->setHorizontalSpacing(2);
    formLayout_2->setVerticalSpacing(2);
    formLayout_2->setContentsMargins(6, 6, 6, 6);
    label_20 = new QLabel(tab_OS_Bootloader);
    label_20->setObjectName("label_20");

    formLayout_2->setWidget(0, QFormLayout::LabelRole, label_20);

    edit_os_bootloader_query = new QLineEdit(tab_OS_Bootloader);
    edit_os_bootloader_query->setObjectName("edit_os_bootloader_query");

    formLayout_2->setWidget(0, QFormLayout::FieldRole, edit_os_bootloader_query);

    label_21 = new QLabel(tab_OS_Bootloader);
    label_21->setObjectName("label_21");

    formLayout_2->setWidget(1, QFormLayout::LabelRole, label_21);

    edit_os_bootloader_response = new QLineEdit(tab_OS_Bootloader);
    edit_os_bootloader_response->setObjectName("edit_os_bootloader_response");
    edit_os_bootloader_response->setReadOnly(true);

    formLayout_2->setWidget(1, QFormLayout::FieldRole, edit_os_bootloader_response);

    selector_OS->addTab(tab_OS_Bootloader, QString());

    gridLayout_7->addWidget(selector_OS, 0, 0, 1, 1);

    tabWidget_2->addTab(tab_OS, QString());
    tab_Stats = new QWidget();
    tab_Stats->setObjectName("tab_Stats");
    gridLayout_11 = new QGridLayout(tab_Stats);
    gridLayout_11->setObjectName("gridLayout_11");
    label_15 = new QLabel(tab_Stats);
    label_15->setObjectName("label_15");

    gridLayout_11->addWidget(label_15, 0, 0, 1, 1);

    combo_STAT_Group = new QComboBox(tab_Stats);
    combo_STAT_Group->setObjectName("combo_STAT_Group");
    combo_STAT_Group->setEditable(true);

    gridLayout_11->addWidget(combo_STAT_Group, 0, 1, 1, 1);

    label_16 = new QLabel(tab_Stats);
    label_16->setObjectName("label_16");

    gridLayout_11->addWidget(label_16, 1, 0, 1, 1);

    table_STAT_Values = new QTableWidget(tab_Stats);
    if (table_STAT_Values->columnCount() < 2)
        table_STAT_Values->setColumnCount(2);
    QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
    table_STAT_Values->setHorizontalHeaderItem(0, __qtablewidgetitem12);
    QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
    table_STAT_Values->setHorizontalHeaderItem(1, __qtablewidgetitem13);
    table_STAT_Values->setObjectName("table_STAT_Values");

    gridLayout_11->addWidget(table_STAT_Values, 1, 1, 1, 1);

    horizontalLayout_9 = new QHBoxLayout();
    horizontalLayout_9->setSpacing(2);
    horizontalLayout_9->setObjectName("horizontalLayout_9");
    radio_STAT_List = new QRadioButton(tab_Stats);
    radio_STAT_List->setObjectName("radio_STAT_List");
    radio_STAT_List->setChecked(true);

    horizontalLayout_9->addWidget(radio_STAT_List);

    radio_STAT_Fetch = new QRadioButton(tab_Stats);
    radio_STAT_Fetch->setObjectName("radio_STAT_Fetch");

    horizontalLayout_9->addWidget(radio_STAT_Fetch);


    gridLayout_11->addLayout(horizontalLayout_9, 2, 0, 1, 2);

    lbl_STAT_Status = new QLabel(tab_Stats);
    lbl_STAT_Status->setObjectName("lbl_STAT_Status");

    gridLayout_11->addWidget(lbl_STAT_Status, 3, 0, 1, 2);

    horizontalLayout_14 = new QHBoxLayout();
    horizontalLayout_14->setSpacing(2);
    horizontalLayout_14->setObjectName("horizontalLayout_14");
    horizontalLayout_14->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_19 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_14->addItem(horizontalSpacer_19);

    btn_STAT_Go = new QPushButton(tab_Stats);
    btn_STAT_Go->setObjectName("btn_STAT_Go");

    horizontalLayout_14->addWidget(btn_STAT_Go);

    horizontalSpacer_20 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_14->addItem(horizontalSpacer_20);


    gridLayout_11->addLayout(horizontalLayout_14, 4, 0, 1, 2);

    tabWidget_2->addTab(tab_Stats, QString());
    tab_Shell = new QWidget();
    tab_Shell->setObjectName("tab_Shell");
    gridLayout_9 = new QGridLayout(tab_Shell);
    gridLayout_9->setSpacing(2);
    gridLayout_9->setObjectName("gridLayout_9");
    verticalLayout_3 = new QVBoxLayout();
    verticalLayout_3->setObjectName("verticalLayout_3");
    btn_SHELL_Clear = new QToolButton(tab_Shell);
    btn_SHELL_Clear->setObjectName("btn_SHELL_Clear");

    verticalLayout_3->addWidget(btn_SHELL_Clear);

    btn_SHELL_Copy = new QToolButton(tab_Shell);
    btn_SHELL_Copy->setObjectName("btn_SHELL_Copy");

    verticalLayout_3->addWidget(btn_SHELL_Copy);


    gridLayout_9->addLayout(verticalLayout_3, 1, 2, 1, 1);

    lbl_SHELL_Status = new QLabel(tab_Shell);
    lbl_SHELL_Status->setObjectName("lbl_SHELL_Status");

    gridLayout_9->addWidget(lbl_SHELL_Status, 2, 0, 1, 3);

    edit_SHELL_Output = new QPlainTextEdit(tab_Shell);
    edit_SHELL_Output->setObjectName("edit_SHELL_Output");
    edit_SHELL_Output->setUndoRedoEnabled(false);
    edit_SHELL_Output->setReadOnly(true);

    gridLayout_9->addWidget(edit_SHELL_Output, 1, 1, 1, 1);

    edit_SHELL_Input = new QLineEdit(tab_Shell);
    edit_SHELL_Input->setObjectName("edit_SHELL_Input");

    gridLayout_9->addWidget(edit_SHELL_Input, 0, 1, 1, 2);

    label_12 = new QLabel(tab_Shell);
    label_12->setObjectName("label_12");

    gridLayout_9->addWidget(label_12, 0, 0, 1, 1);

    horizontalLayout_8 = new QHBoxLayout();
    horizontalLayout_8->setSpacing(2);
    horizontalLayout_8->setObjectName("horizontalLayout_8");
    horizontalLayout_8->setContentsMargins(-1, -1, -1, 0);
    horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_8->addItem(horizontalSpacer_7);

    btn_SHELL_Go = new QPushButton(tab_Shell);
    btn_SHELL_Go->setObjectName("btn_SHELL_Go");

    horizontalLayout_8->addWidget(btn_SHELL_Go);

    horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_8->addItem(horizontalSpacer_8);


    gridLayout_9->addLayout(horizontalLayout_8, 3, 0, 1, 3);

    label_13 = new QLabel(tab_Shell);
    label_13->setObjectName("label_13");

    gridLayout_9->addWidget(label_13, 1, 0, 1, 1);

    tabWidget_2->addTab(tab_Shell, QString());
    tab_Settings = new QWidget();
    tab_Settings->setObjectName("tab_Settings");
    tabWidget_2->addTab(tab_Settings, QString());

    verticalLayout_2->addWidget(tabWidget_2);

//    tabWidget->addTab(tab, QString());
    tab_2 = new QWidget();
    tab_2->setObjectName("tab_2");
    verticalLayoutWidget = new QWidget(tab_2);
    verticalLayoutWidget->setObjectName("verticalLayoutWidget");
    verticalLayoutWidget->setGeometry(QRect(6, 6, 229, 182));
    verticalLayout = new QVBoxLayout(verticalLayoutWidget);
    verticalLayout->setSpacing(2);
    verticalLayout->setObjectName("verticalLayout");
    verticalLayout->setContentsMargins(6, 6, 8, 6);
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

//    tabWidget->addTab(tab_2, QString());

//    gridLayout->addWidget(tabWidget, 0, 0, 1, 1);


//    retranslateUi(Form);

//    tabWidget->setCurrentIndex(0);
    tabWidget_2->setCurrentIndex(1);
    tabWidget_3->setCurrentIndex(0);
    selector_OS->setCurrentIndex(0);
///AUTOGEN_END_INIT

    //retranslate code
///AUTOGEN_START_TRANSLATE
//    Form->setWindowTitle(QCoreApplication::translate("Form", "Form", nullptr));
    label->setText(QCoreApplication::translate("Form", "MTU:", nullptr));
    check_V2_Protocol->setText(QCoreApplication::translate("Form", "v2 protocol", nullptr));
    radio_transport_uart->setText(QCoreApplication::translate("Form", "UART", nullptr));
    radio_transport_udp->setText(QCoreApplication::translate("Form", "UDP", nullptr));
    radio_transport_bluetooth->setText(QCoreApplication::translate("Form", "Bluetooth", nullptr));
    btn_transport_connect->setText(QCoreApplication::translate("Form", "Connect", nullptr));
    btn_FS_Local->setText(QCoreApplication::translate("Form", "...", nullptr));
    btn_FS_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    radio_FS_Upload->setText(QCoreApplication::translate("Form", "Upload", nullptr));
    radio_FS_Download->setText(QCoreApplication::translate("Form", "Download", nullptr));
    radio_FS_Size->setText(QCoreApplication::translate("Form", "Size", nullptr));
    radio_FS_HashChecksum->setText(QCoreApplication::translate("Form", "Hash/checksum", nullptr));
    radio_FS_Hash_Checksum_Types->setText(QCoreApplication::translate("Form", "Types", nullptr));
    lbl_FS_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    label_2->setText(QCoreApplication::translate("Form", "Local file:", nullptr));
    label_3->setText(QCoreApplication::translate("Form", "Device file:", nullptr));
    label_19->setText(QCoreApplication::translate("Form", "Hash/checksum:", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_FS), QCoreApplication::translate("Form", "FS", nullptr));
    label_4->setText(QCoreApplication::translate("Form", "File:", nullptr));
    check_IMG_Reset->setText(QCoreApplication::translate("Form", "After upload", nullptr));
    label_6->setText(QCoreApplication::translate("Form", "Progress:", nullptr));
    btn_IMG_Local->setText(QCoreApplication::translate("Form", "...", nullptr));
    label_41->setText(QCoreApplication::translate("Form", "Image:", nullptr));
    radio_IMG_No_Action->setText(QCoreApplication::translate("Form", "No action", nullptr));
    radio_IMG_Test->setText(QCoreApplication::translate("Form", "Test", nullptr));
    radio_IMG_Confirm->setText(QCoreApplication::translate("Form", "Confirm", nullptr));
    label_9->setText(QCoreApplication::translate("Form", "Reset:", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Upload), QCoreApplication::translate("Form", "Upload", nullptr));
    label_5->setText(QCoreApplication::translate("Form", "State:", nullptr));
    radio_IMG_Get->setText(QCoreApplication::translate("Form", "Get", nullptr));
    radio_IMG_Set->setText(QCoreApplication::translate("Form", "Set", nullptr));
    check_IMG_Confirm->setText(QCoreApplication::translate("Form", "Confirm", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Images), QCoreApplication::translate("Form", "Images", nullptr));
    label_14->setText(QCoreApplication::translate("Form", "Slot:", nullptr));
    tabWidget_3->setTabText(tabWidget_3->indexOf(tab_IMG_Erase), QCoreApplication::translate("Form", "Erase", nullptr));
    btn_IMG_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    lbl_IMG_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_IMG), QCoreApplication::translate("Form", "Img", nullptr));
    btn_OS_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    lbl_OS_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    label_10->setText(QCoreApplication::translate("Form", "Input:", nullptr));
    label_11->setText(QCoreApplication::translate("Form", "Output:", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Echo), QCoreApplication::translate("Form", "Echo", nullptr));
    QTableWidgetItem *___qtablewidgetitem = table_OS_Tasks->horizontalHeaderItem(0);
    ___qtablewidgetitem->setText(QCoreApplication::translate("Form", "Task", nullptr));
    QTableWidgetItem *___qtablewidgetitem1 = table_OS_Tasks->horizontalHeaderItem(1);
    ___qtablewidgetitem1->setText(QCoreApplication::translate("Form", "ID", nullptr));
    QTableWidgetItem *___qtablewidgetitem2 = table_OS_Tasks->horizontalHeaderItem(2);
    ___qtablewidgetitem2->setText(QCoreApplication::translate("Form", "Priority", nullptr));
    QTableWidgetItem *___qtablewidgetitem3 = table_OS_Tasks->horizontalHeaderItem(3);
    ___qtablewidgetitem3->setText(QCoreApplication::translate("Form", "State", nullptr));
    QTableWidgetItem *___qtablewidgetitem4 = table_OS_Tasks->horizontalHeaderItem(4);
    ___qtablewidgetitem4->setText(QCoreApplication::translate("Form", "Context Switches", nullptr));
    QTableWidgetItem *___qtablewidgetitem5 = table_OS_Tasks->horizontalHeaderItem(5);
    ___qtablewidgetitem5->setText(QCoreApplication::translate("Form", "Runtime", nullptr));
    QTableWidgetItem *___qtablewidgetitem6 = table_OS_Tasks->horizontalHeaderItem(6);
    ___qtablewidgetitem6->setText(QCoreApplication::translate("Form", "Stack size", nullptr));
    QTableWidgetItem *___qtablewidgetitem7 = table_OS_Tasks->horizontalHeaderItem(7);
    ___qtablewidgetitem7->setText(QCoreApplication::translate("Form", "Stack usage", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Tasks), QCoreApplication::translate("Form", "Tasks", nullptr));
    QTableWidgetItem *___qtablewidgetitem8 = table_OS_Memory->horizontalHeaderItem(0);
    ___qtablewidgetitem8->setText(QCoreApplication::translate("Form", "Name", nullptr));
    QTableWidgetItem *___qtablewidgetitem9 = table_OS_Memory->horizontalHeaderItem(1);
    ___qtablewidgetitem9->setText(QCoreApplication::translate("Form", "Size", nullptr));
    QTableWidgetItem *___qtablewidgetitem10 = table_OS_Memory->horizontalHeaderItem(2);
    ___qtablewidgetitem10->setText(QCoreApplication::translate("Form", "Free", nullptr));
    QTableWidgetItem *___qtablewidgetitem11 = table_OS_Memory->horizontalHeaderItem(3);
    ___qtablewidgetitem11->setText(QCoreApplication::translate("Form", "Minimum", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Memory), QCoreApplication::translate("Form", "Memory", nullptr));
    check_OS_Force_Reboot->setText(QCoreApplication::translate("Form", "Force reboot", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Reset), QCoreApplication::translate("Form", "Reset", nullptr));
    label_17->setText(QCoreApplication::translate("Form", "uname:", nullptr));
    radio_OS_Buffer_Info->setText(QCoreApplication::translate("Form", "Buffer info", nullptr));
    radio_OS_uname->setText(QCoreApplication::translate("Form", "uname", nullptr));
    label_18->setText(QCoreApplication::translate("Form", "Output:", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Info), QCoreApplication::translate("Form", "Info", nullptr));
    label_20->setText(QCoreApplication::translate("Form", "Query (blank for bootloader):", nullptr));
    edit_os_bootloader_query->setPlaceholderText(QCoreApplication::translate("Form", "Bootloader", nullptr));
    label_21->setText(QCoreApplication::translate("Form", "Response:", nullptr));
    selector_OS->setTabText(selector_OS->indexOf(tab_OS_Bootloader), QCoreApplication::translate("Form", "Bootloader", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_OS), QCoreApplication::translate("Form", "OS", nullptr));
    label_15->setText(QCoreApplication::translate("Form", "Group:", nullptr));
    label_16->setText(QCoreApplication::translate("Form", "Values:", nullptr));
    QTableWidgetItem *___qtablewidgetitem12 = table_STAT_Values->horizontalHeaderItem(0);
    ___qtablewidgetitem12->setText(QCoreApplication::translate("Form", "Name", nullptr));
    QTableWidgetItem *___qtablewidgetitem13 = table_STAT_Values->horizontalHeaderItem(1);
    ___qtablewidgetitem13->setText(QCoreApplication::translate("Form", "Value", nullptr));
    radio_STAT_List->setText(QCoreApplication::translate("Form", "List Groups", nullptr));
    radio_STAT_Fetch->setText(QCoreApplication::translate("Form", "Fetch Stats", nullptr));
    lbl_STAT_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    btn_STAT_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_Stats), QCoreApplication::translate("Form", "Stats", nullptr));
    btn_SHELL_Clear->setText(QCoreApplication::translate("Form", "Clear", nullptr));
    btn_SHELL_Copy->setText(QCoreApplication::translate("Form", "Copy", nullptr));
    lbl_SHELL_Status->setText(QCoreApplication::translate("Form", "[Status]", nullptr));
    label_12->setText(QCoreApplication::translate("Form", "Input:", nullptr));
    btn_SHELL_Go->setText(QCoreApplication::translate("Form", "Go", nullptr));
    label_13->setText(QCoreApplication::translate("Form", "Output:", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_Shell), QCoreApplication::translate("Form", "Shell", nullptr));
    tabWidget_2->setTabText(tabWidget_2->indexOf(tab_Settings), QCoreApplication::translate("Form", "Settings", nullptr));
//    tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("Form", "MCUmgr", nullptr));
    label_7->setText(QCoreApplication::translate("Form", "Hash:", nullptr));
    label_8->setText(QCoreApplication::translate("Form", "Version:", nullptr));
    check_IMG_Preview_Confirmed->setText(QCoreApplication::translate("Form", "Confirmed", nullptr));
    check_IMG_Preview_Active->setText(QCoreApplication::translate("Form", "Active", nullptr));
    check_IMG_Preview_Pending->setText(QCoreApplication::translate("Form", "Pending", nullptr));
    check_IMG_Preview_Bootable->setText(QCoreApplication::translate("Form", "Bootable", nullptr));
    check_IMG_Preview_Permanent->setText(QCoreApplication::translate("Form", "Permanent", nullptr));
    btn_IMG_Preview_Copy->setText(QCoreApplication::translate("Form", "Copy", nullptr));
//    tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("Form", "Page", nullptr));
///AUTOGEN_END_TRANSLATE

    //Add code
    tabWidget_orig->addTab(tab, QString("MCUmgr"));

//Signals
    connect(uart_transport, SIGNAL(serial_write(QByteArray*)), parent_window, SLOT(plugin_serial_transmit(QByteArray*)));
    //connect(uart, SIGNAL(receive_waiting(QByteArray)), this, SLOT(receive_waiting(QByteArray)));
    connect(uart_transport, SIGNAL(receive_waiting(smp_message*)), processor, SLOT(message_received(smp_message*)));
    //connect(btn_IMG_Local, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Local_clicked()));
    //connect(btn_IMG_Go, SIGNAL(clicked()), this, SLOT(on_btn_IMG_Go_clicked()));

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    connect(udp_transport, SIGNAL(receive_waiting(smp_message*)), processor, SLOT(message_received(smp_message*)));
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    connect(bluetooth_transport, SIGNAL(receive_waiting(smp_message*)), processor, SLOT(message_received(smp_message*)));
#endif

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
    connect(btn_transport_connect, SIGNAL(clicked()), this, SLOT(on_btn_transport_connect_clicked()));
    connect(colview_IMG_Images, SIGNAL(updatePreviewWidget(QModelIndex)), this, SLOT(on_colview_IMG_Images_updatePreviewWidget(QModelIndex)));
    connect(radio_transport_uart, SIGNAL(toggled(bool)), this, SLOT(on_radio_transport_uart_toggled(bool)));
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    connect(radio_transport_udp, SIGNAL(toggled(bool)), this, SLOT(on_radio_transport_udp_toggled(bool)));
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    connect(radio_transport_bluetooth, SIGNAL(toggled(bool)), this, SLOT(on_radio_transport_bluetooth_toggled(bool)));
#endif
    connect(radio_OS_Buffer_Info, SIGNAL(toggled(bool)), this, SLOT(on_radio_OS_Buffer_Info_toggled(bool)));
    connect(radio_OS_uname, SIGNAL(toggled(bool)), this, SLOT(on_radio_OS_uname_toggled(bool)));
    connect(radio_IMG_Get, SIGNAL(toggled(bool)), this, SLOT(on_radio_IMG_Get_toggled(bool)));
    connect(radio_IMG_Set, SIGNAL(toggled(bool)), this, SLOT(on_radio_IMG_Set_toggled(bool)));

    colview_IMG_Images->setModel(&model_image_state);
    colview_IMG_Images->setColumnWidths(QList<int>() << 50 << 50 << 460);

    check_IMG_Preview_Confirmed->setChecked(true);
    check_IMG_Preview_Confirmed->installEventFilter(this);
    check_IMG_Preview_Active->installEventFilter(this);
    check_IMG_Preview_Pending->installEventFilter(this);
    check_IMG_Preview_Bootable->installEventFilter(this);
    check_IMG_Preview_Permanent->installEventFilter(this);


    //test
//    emit plugin_add_open_close_button(btn_FS_Go);
    smp_groups.img_mgmt = new smp_group_img_mgmt(processor);
    smp_groups.os_mgmt = new smp_group_os_mgmt(processor);
    smp_groups.shell_mgmt = new smp_group_shell_mgmt(processor);
    smp_groups.stat_mgmt = new smp_group_stat_mgmt(processor);
    smp_groups.fs_mgmt = new smp_group_fs_mgmt(processor);
    error_lookup_form = new error_lookup(parent_window, &smp_groups);

    //error_lookup_form->show();

    connect(smp_groups.img_mgmt, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
    connect(smp_groups.img_mgmt, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
    connect(smp_groups.img_mgmt, SIGNAL(plugin_to_hex(QByteArray*)), this, SLOT(group_to_hex(QByteArray*)));

    connect(smp_groups.os_mgmt, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
    connect(smp_groups.os_mgmt, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
//    connect(smp_groups.os_mgmt, SIGNAL(plugin_to_hex(QByteArray*)), this, SLOT(group_to_hex(QByteArray*)));

    connect(smp_groups.shell_mgmt, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
    connect(smp_groups.shell_mgmt, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
//    connect(smp_groups.shell_mgmt, SIGNAL(plugin_to_hex(QByteArray*)), this, SLOT(group_to_hex(QByteArray*)));

    connect(smp_groups.stat_mgmt, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
    connect(smp_groups.stat_mgmt, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
//    connect(smp_groups.shell_mgmt, SIGNAL(plugin_to_hex(QByteArray*)), this, SLOT(group_to_hex(QByteArray*)));

    connect(smp_groups.fs_mgmt, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
    connect(smp_groups.fs_mgmt, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
//    connect(smp_groups.fs_mgmt, SIGNAL(plugin_to_hex(QByteArray*)), this, SLOT(group_to_hex(QByteArray*)));
}

plugin_mcumgr::~plugin_mcumgr()
{
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    delete bluetooth_transport;
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    delete udp_transport;
#endif

    delete error_lookup_form;
    delete smp_groups.fs_mgmt;
    delete smp_groups.stat_mgmt;
    delete smp_groups.shell_mgmt;
    delete smp_groups.os_mgmt;
    delete smp_groups.img_mgmt;
    delete processor;
    delete uart_transport;
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
    return "AuTerm MCUmgr plugin\r\nCopyright 2021-2023 Jamie M.\r\n\r\nCan be used to communicate with Zephyr devices with the serial"
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
           "/UDP"
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
           "/Bluetooth"
#endif
           " MCUmgr transport"
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP) || defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
           "s"
#endif
           " enabled.\r\n\r\nUNFINISHED INITIAL TEST USE ONLY, NOT REPRESENTATIVE OF FINAL PRODUCT.\r\n\r\nBuilt using Qt " QT_VERSION_STR;
}

bool plugin_mcumgr::plugin_configuration()
{
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
    uart_transport->serial_read(data);
}

void plugin_mcumgr::serial_bytes_written(qint64 bytes)
{
    Q_UNUSED(bytes);
}

void plugin_mcumgr::serial_about_to_close()
{
}

void plugin_mcumgr::serial_opened()
{
}

void plugin_mcumgr::serial_closed()
{
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP) || defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    if (active_transport() != uart_transport)
    {
        return;
    }
#endif

    switch (mode)
    {
        case ACTION_IMG_UPLOAD:
        case ACTION_IMG_UPLOAD_SET:
        case ACTION_IMG_IMAGE_LIST:
        case ACTION_IMG_IMAGE_SET:
        case ACTION_IMG_IMAGE_ERASE:
        {
            smp_groups.img_mgmt->cancel();
            break;
        }

        case ACTION_OS_UPLOAD_RESET:
        case ACTION_OS_ECHO:
        case ACTION_OS_TASK_STATS:
        case ACTION_OS_MEMORY_POOL:
        case ACTION_OS_RESET:
        case ACTION_OS_MCUMGR_BUFFER:
        case ACTION_OS_OS_APPLICATION_INFO:
        case ACTION_OS_BOOTLOADER_INFO:
        {
            smp_groups.os_mgmt->cancel();
            break;
        }

        case ACTION_SHELL_EXECUTE:
        {
            smp_groups.shell_mgmt->cancel();
            break;
        }

        case ACTION_STAT_GROUP_DATA:
        case ACTION_STAT_LIST_GROUPS:
        {
            smp_groups.stat_mgmt->cancel();
            break;
        }

        case ACTION_FS_UPLOAD:
        case ACTION_FS_DOWNLOAD:
        case ACTION_FS_STATUS:
        case ACTION_FS_HASH_CHECKSUM:
        case ACTION_FS_SUPPORTED_HASHES_CHECKSUMS:
        {
            smp_groups.fs_mgmt->cancel();
            break;
        }

        default:
        {
        }
    }

    mode = ACTION_IDLE;
}

//Form actions
void plugin_mcumgr::on_btn_FS_Local_clicked()
{
    QString filename;

    if (radio_FS_Upload->isChecked())
    {
        //TODO: load path
        filename = QFileDialog::getOpenFileName(parent_window, "Select source/target file for transfer", "", "All Files (*)");
    }
    else
    {
        //TODO: load path
        filename = QFileDialog::getSaveFileName(parent_window, "Select source/target file for transfer", "", "All Files (*)");
    }

    if (!filename.isEmpty())
    {
        edit_FS_Local->setText(filename);
    }
}

void plugin_mcumgr::on_btn_FS_Go_clicked()
{
    bool started = false;
    bool reset_status = false;

    if (claim_transport(lbl_FS_Status) == false)
    {
        return;
    }

    if (radio_FS_Upload->isChecked())
    {
        mode = ACTION_FS_UPLOAD;
        processor->set_transport(active_transport());
        smp_groups.fs_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.fs_mgmt->start_upload(edit_FS_Local->text(), edit_FS_Remote->text());

        if (started == true)
        {
            lbl_FS_Status->setText("Uploading...");
        }
    }
    else if (radio_FS_Download->isChecked())
    {
        mode = ACTION_FS_DOWNLOAD;
        processor->set_transport(active_transport());
        smp_groups.fs_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.fs_mgmt->start_download(edit_FS_Remote->text(), edit_FS_Local->text());

        if (started == true)
        {
            lbl_FS_Status->setText("Downloading...");
        }
    }
    else if (radio_FS_Size->isChecked())
    {
        mode = ACTION_FS_STATUS;
        processor->set_transport(active_transport());
        smp_groups.fs_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.fs_mgmt->start_status(edit_FS_Remote->text());

        if (started == true)
        {
            lbl_FS_Status->setText("Statusing...");
        }
    }
    else if (radio_FS_HashChecksum->isChecked())
    {
        mode = ACTION_FS_HASH_CHECKSUM;
        processor->set_transport(active_transport());
        smp_groups.fs_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.fs_mgmt->start_hash_checksum(edit_FS_Remote->text(), combo_FS_Hash_Checksum->currentText());

        if (started == true)
        {
            lbl_FS_Status->setText("Hashing...");
        }
    }
    else if (radio_FS_Hash_Checksum_Types->isChecked())
    {
        mode = ACTION_FS_SUPPORTED_HASHES_CHECKSUMS;
        processor->set_transport(active_transport());
        smp_groups.fs_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.fs_mgmt->start_supported_hashes_checksums(&supported_hash_checksum_list);

        if (started == true)
        {
            lbl_FS_Status->setText("Supported...");
        }
    }

    if (started == true)
    {
        progress_FS_Complete->setValue(0);
    }
    else if (reset_status == true)
    {
        relase_transport();
    }
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
    QString strFilename = QFileDialog::getOpenFileName(parent_window, tr("Open firmware file"), edit_IMG_Local->text(), tr("Binary Files (*.bin);;All Files (*)"));

    if (!strFilename.isEmpty())
    {
        edit_IMG_Local->setText(strFilename);
    }
}

void plugin_mcumgr::on_btn_IMG_Go_clicked()
{
    bool started = false;
    bool reset_status = false;

    if (claim_transport(lbl_IMG_Status) == false)
    {
        return;
    }

    if (tabWidget_3->currentWidget() == tab_IMG_Upload)
    {
        //Upload
        mode = ACTION_IMG_UPLOAD;
        processor->set_transport(active_transport());
        smp_groups.img_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.img_mgmt->start_firmware_update(edit_IMG_Image->value(), edit_IMG_Local->text(), false, &upload_hash);

        if (started == true)
        {
            lbl_IMG_Status->setText("Uploading...");
        }
    }
    else if (tabWidget_3->currentWidget() == tab_IMG_Images)
    {
        //Image list
        if (radio_IMG_Get->isChecked())
        {
            colview_IMG_Images->previewWidget()->hide();
            model_image_state.clear();
            images_list.clear();
            mode = ACTION_IMG_IMAGE_LIST;
            processor->set_transport(active_transport());
            smp_groups.img_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
            started = smp_groups.img_mgmt->start_image_get(&images_list);

            if (started == true)
            {
                lbl_IMG_Status->setText("Querying...");
            }
        }
        else
        {
            if (colview_IMG_Images->currentIndex().isValid() && colview_IMG_Images->currentIndex().parent().isValid())
            {
                uint8_t i = 0;
                uint8_t l = 0;
                bool found = false;

                while (i < images_list.length())
                {
                    if (images_list[i].item == model_image_state.itemFromIndex(colview_IMG_Images->currentIndex())->parent())
                    {
                        l = 0;
                        while (l < images_list[i].slot_list.length())
                        {
                            if (model_image_state.itemFromIndex(colview_IMG_Images->currentIndex()) == images_list[i].slot_list[l].item)
                            {
                                found = true;
                                goto finished;
                            }

                            ++l;
                        }
                    }

                    ++i;
                }

finished:
                if (found == true)
                {
                    mode = ACTION_IMG_IMAGE_SET;
                    processor->set_transport(active_transport());
                    smp_groups.img_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);

                    parent_row = colview_IMG_Images->currentIndex().parent().row();
                    parent_column = colview_IMG_Images->currentIndex().parent().column();
                    child_row = colview_IMG_Images->currentIndex().row();
                    child_column = colview_IMG_Images->currentIndex().column();
                    started = smp_groups.img_mgmt->start_image_set(&images_list[i].slot_list[l].hash, check_IMG_Confirm->isChecked(), &images_list);

                    if (started == true)
                    {
                        lbl_IMG_Status->setText("Setting...");
                    }
                }
                else
                {
                    lbl_IMG_Status->setText("Could not find item bounds");
                    started = false;
                    reset_status = true;
                }
            }
            else
            {
                lbl_IMG_Status->setText("Invalid selection");
                started = false;
                reset_status = true;
            }
        }
    }
    else if (tabWidget_3->currentWidget() == tab_IMG_Erase)
    {
        //Erase
        mode = ACTION_IMG_IMAGE_ERASE;
        processor->set_transport(active_transport());
        smp_groups.img_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_erase_ms, mode);
        started = smp_groups.img_mgmt->start_image_erase(edit_IMG_Erase_Slot->value());

        if (started == true)
        {
            lbl_IMG_Status->setText("Erasing...");
        }
    }

    if (started == true)
    {
        progress_IMG_Complete->setValue(0);
    }
    else if (reset_status == true)
    {
        relase_transport();
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
    bool started = false;
    bool reset_status = false;

    if (claim_transport(lbl_OS_Status) == false)
    {
        return;
    }

    if (selector_OS->currentWidget() == tab_OS_Echo)
    {
        edit_OS_Echo_Output->clear();
        mode = ACTION_OS_ECHO;
        processor->set_transport(active_transport());
        smp_groups.os_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.os_mgmt->start_echo(edit_OS_Echo_Input->toPlainText());

        if (started == true)
        {
            lbl_OS_Status->setText("Echo command sent...");
        }
        else
        {
            reset_status = true;
        }
    }
    else if (selector_OS->currentWidget() == tab_OS_Tasks)
    {
        mode = ACTION_OS_TASK_STATS;
        processor->set_transport(active_transport());
        smp_groups.os_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.os_mgmt->start_task_stats(&task_list);

        if (started == true)
        {
            lbl_OS_Status->setText("Task list command sent...");
        }
        else
        {
            reset_status = true;
        }
    }
    else if (selector_OS->currentWidget() == tab_OS_Memory)
    {
        mode = ACTION_OS_MEMORY_POOL;
        processor->set_transport(active_transport());
        smp_groups.os_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.os_mgmt->start_memory_pool(&memory_list);

        if (started == true)
        {
            lbl_OS_Status->setText("Memory pool list command sent...");
        }
        else
        {
            reset_status = true;
        }
    }
    else if (selector_OS->currentWidget() == tab_OS_Reset)
    {
        mode = ACTION_OS_RESET;
        processor->set_transport(active_transport());
        smp_groups.os_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.os_mgmt->start_reset(check_OS_Force_Reboot->isChecked());

        if (started == true)
        {
            lbl_OS_Status->setText("Reset command...");
        }
        else
        {
            reset_status = true;
        }
    }
    else if (selector_OS->currentWidget() == tab_OS_Info)
    {
        if (radio_OS_uname->isChecked())
        {
            //uname
            mode = ACTION_OS_OS_APPLICATION_INFO;
            processor->set_transport(active_transport());
            smp_groups.os_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
            started = smp_groups.os_mgmt->start_os_application_info(edit_OS_UName->text());

            if (started == true)
            {
                lbl_OS_Status->setText("uname command sent...");
            }
            else
            {
                reset_status = true;
            }
        }
        else
        {
            //Buffer details
            mode = ACTION_OS_MCUMGR_BUFFER;
            processor->set_transport(active_transport());
            smp_groups.os_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
            started = smp_groups.os_mgmt->start_mcumgr_parameters();

            if (started == true)
            {
                lbl_OS_Status->setText("MCUmgr buffer command sent...");
            }
            else
            {
                reset_status = true;
            }
        }
    }
    else if (selector_OS->currentWidget() == tab_OS_Bootloader)
    {
        //bootloader info
        mode = ACTION_OS_BOOTLOADER_INFO;
        processor->set_transport(active_transport());
        smp_groups.os_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.os_mgmt->start_bootloader_info(edit_os_bootloader_query->text(), &bootloader_info_response);

        if (started == true)
        {
            lbl_OS_Status->setText("bootloader info command sent...");
        }
        else
        {
            reset_status = true;
        }
    }

    if (reset_status == true)
    {
        relase_transport();
    }
}

void plugin_mcumgr::on_btn_SHELL_Go_clicked()
{
    //Execute shell command
    bool started = false;
    bool reset_status = false;

    if (claim_transport(lbl_SHELL_Status) == false)
    {
        return;
    }

    QRegularExpression reTempRE("\\s+");
    QStringList list_arguments = edit_SHELL_Input->text().split(reTempRE);

    mode = ACTION_SHELL_EXECUTE;
    processor->set_transport(active_transport());
    smp_groups.shell_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
    started = smp_groups.shell_mgmt->start_execute(&list_arguments, &shell_rc);

    if (started == true)
    {
        lbl_SHELL_Status->setText("Shell execute command sent...");
    }
    else
    {
        relase_transport();
    }
}

void plugin_mcumgr::on_btn_STAT_Go_clicked()
{
    bool started = false;
    bool reset_status = false;

    if (claim_transport(lbl_STAT_Status) == false)
    {
        return;
    }

    if (radio_STAT_List->isChecked())
    {
        //Execute stat list command
        mode = ACTION_STAT_LIST_GROUPS;
        processor->set_transport(active_transport());
        smp_groups.stat_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.stat_mgmt->start_list_groups(&group_list);

        if (started == true)
        {
            lbl_STAT_Status->setText("Stat list command sent...");
        }
    }
    else if (radio_STAT_Fetch->isChecked() && !combo_STAT_Group->currentText().isEmpty())
    {
        //Execute stat get command
        mode = ACTION_STAT_GROUP_DATA;
        processor->set_transport(active_transport());
        smp_groups.stat_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
        started = smp_groups.stat_mgmt->start_group_data(combo_STAT_Group->currentText(), &stat_list);

        if (started == true)
        {
            lbl_STAT_Status->setText("Stat get command sent...");
        }
    }

    if (reset_status == true)
    {
        relase_transport();
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
    while (i < images_list.length())
    {
        if (images_list[i].item == model_image_state.itemFromIndex(index)->parent())
        {
            uint8_t l = 0;
            while (l < images_list[i].slot_list.length())
            {
                if (model_image_state.itemFromIndex(index) == images_list[i].slot_list[l].item)
                {
                    QByteArray escaped_hash = images_list[i].slot_list[l].hash;
                    emit plugin_to_hex(&escaped_hash);
                    edit_IMG_Preview_Hash->setText(escaped_hash);
                    edit_IMG_Preview_Version->setText(images_list[i].slot_list[l].version);
                    check_IMG_Preview_Active->setChecked(images_list[i].slot_list[l].active);
                    check_IMG_Preview_Bootable->setChecked(images_list[i].slot_list[l].bootable);
                    check_IMG_Preview_Confirmed->setChecked(images_list[i].slot_list[l].confirmed);
                    check_IMG_Preview_Pending->setChecked(images_list[i].slot_list[l].pending);
                    check_IMG_Preview_Permanent->setChecked(images_list[i].slot_list[l].permanent);

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

    QLabel *label_status = nullptr;
    bool finished = true;

    qDebug() << "Status: " << status << "Sender: " << sender();
    if (sender() == smp_groups.img_mgmt)
    {
        qDebug() << "img sender";
        label_status = lbl_IMG_Status;

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

                    mode = ACTION_IMG_UPLOAD_SET;
                    processor->set_transport(active_transport());
                    smp_groups.img_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
                    bool started = smp_groups.img_mgmt->start_image_set(&upload_hash, (radio_IMG_Confirm->isChecked() ? true : false), nullptr);
//todo: check status
                    qDebug() << "do upload of " << upload_hash;
                }
            }
            else if (user_data == ACTION_IMG_UPLOAD_SET)
            {
                if (check_IMG_Reset->isChecked())
                {
                    //Reboot device
                    finished = false;

                    mode = ACTION_OS_UPLOAD_RESET;
                    processor->set_transport(active_transport());
                    smp_groups.os_mgmt->set_parameters((check_V2_Protocol->isChecked() ? 1 : 0), edit_MTU->value(), retries, timeout_ms, mode);
                    bool started = smp_groups.os_mgmt->start_reset(false);
//todo: check status
                    qDebug() << "do reset";
                }
            }
            else if (user_data == ACTION_IMG_IMAGE_LIST)
            {
                uint8_t i = 0;
                while (i < images_list.length())
                {
                    model_image_state.appendRow(images_list[i].item);
                    ++i;
                }
            }
            else if (user_data == ACTION_IMG_IMAGE_SET)
            {
                if (parent_row != -1 && parent_column != -1 && child_row != -1 && child_column != -1)
                {
                    uint8_t i = 0;

                    model_image_state.clear();

                    while (i < images_list.length())
                    {
                        model_image_state.appendRow(images_list[i].item);
                        ++i;
                    }

                    if (model_image_state.hasIndex(parent_row, parent_column) == true && model_image_state.index(child_row, child_column, model_image_state.index(parent_row, parent_column)).isValid() == true)
                    {
                        colview_IMG_Images->setCurrentIndex(model_image_state.index(child_row, child_column, model_image_state.index(parent_row, parent_column)));
                    }
                    else
                    {
                        colview_IMG_Images->previewWidget()->hide();
                    }

                    parent_row = -1;
                    parent_column = -1;
                    child_row = -1;
                    child_column = -1;
                }
                else
                {
                    colview_IMG_Images->previewWidget()->hide();
                }
            }
        }
    }
    else if (sender() == smp_groups.os_mgmt)
    {
        qDebug() << "os sender";
        label_status = lbl_OS_Status;

        if (status == STATUS_COMPLETE)
        {
            qDebug() << "complete";
            if (user_data == ACTION_OS_ECHO)
            {
                edit_OS_Echo_Output->appendPlainText(error_string);
                error_string = nullptr;
            }
            else if (user_data == ACTION_OS_UPLOAD_RESET)
            {
            }
            else if (user_data == ACTION_OS_MEMORY_POOL)
            {
                uint16_t i = 0;
                uint16_t l = table_OS_Memory->rowCount();

                table_OS_Memory->setSortingEnabled(false);

                while (i < memory_list.length())
                {
                    if (i >= l)
                    {
                        table_OS_Memory->insertRow(i);

                        QTableWidgetItem *row_name = new QTableWidgetItem(memory_list[i].name);
                        QTableWidgetItem *row_size = new QTableWidgetItem(QString::number(memory_list[i].blocks * memory_list[i].size));
                        QTableWidgetItem *row_free = new QTableWidgetItem(QString::number(memory_list[i].free * memory_list[i].size));
                        QTableWidgetItem *row_minimum = new QTableWidgetItem(QString::number(memory_list[i].minimum * memory_list[i].size));

                        table_OS_Memory->setItem(i, 0, row_name);
                        table_OS_Memory->setItem(i, 1, row_size);
                        table_OS_Memory->setItem(i, 2, row_free);
                        table_OS_Memory->setItem(i, 3, row_minimum);
                    }
                    else
                    {
                        table_OS_Memory->item(i, 0)->setText(memory_list[i].name);
                        table_OS_Memory->item(i, 1)->setText(QString::number(memory_list[i].blocks * memory_list[i].size));
                        table_OS_Memory->item(i, 2)->setText(QString::number(memory_list[i].free * memory_list[i].size));
                        table_OS_Memory->item(i, 3)->setText(QString::number(memory_list[i].minimum * memory_list[i].size));
                    }

                    ++i;
                }

                while (i < l)
                {
                    table_OS_Memory->removeRow((table_OS_Memory->rowCount() - 1));
                    ++i;
                }

                table_OS_Memory->setSortingEnabled(true);
            }
            else if (user_data == ACTION_OS_TASK_STATS)
            {
                uint16_t i = 0;
                uint16_t l = table_OS_Tasks->rowCount();

                table_OS_Tasks->setSortingEnabled(false);

                while (i < task_list.length())
                {
                    if (i >= l)
                    {
                        table_OS_Tasks->insertRow(i);

                        QTableWidgetItem *row_name = new QTableWidgetItem(task_list[i].name);
                        QTableWidgetItem *row_id = new QTableWidgetItem(QString::number(task_list[i].id));
                        QTableWidgetItem *row_priority = new QTableWidgetItem(QString::number(task_list[i].priority));
                        QTableWidgetItem *row_state = new QTableWidgetItem(QString::number(task_list[i].state));
                        QTableWidgetItem *row_context_switches = new QTableWidgetItem(QString::number(task_list[i].context_switches));
                        QTableWidgetItem *row_runtime = new QTableWidgetItem(QString::number(task_list[i].runtime));
                        QTableWidgetItem *row_stack_size = new QTableWidgetItem(QString::number(task_list[i].stack_size * 4));
                        QTableWidgetItem *row_stack_usage = new QTableWidgetItem(QString::number(task_list[i].stack_usage * 4));

                        table_OS_Tasks->setItem(i, 0, row_name);
                        table_OS_Tasks->setItem(i, 1, row_id);
                        table_OS_Tasks->setItem(i, 2, row_priority);
                        table_OS_Tasks->setItem(i, 3, row_state);
                        table_OS_Tasks->setItem(i, 4, row_context_switches);
                        table_OS_Tasks->setItem(i, 5, row_runtime);
                        table_OS_Tasks->setItem(i, 6, row_stack_size);
                        table_OS_Tasks->setItem(i, 7, row_stack_usage);
                    }
                    else
                    {
                        table_OS_Tasks->item(i, 0)->setText(task_list[i].name);
                        table_OS_Tasks->item(i, 1)->setText(QString::number(task_list[i].id));
                        table_OS_Tasks->item(i, 2)->setText(QString::number(task_list[i].priority));
                        table_OS_Tasks->item(i, 3)->setText(QString::number(task_list[i].state));
                        table_OS_Tasks->item(i, 4)->setText(QString::number(task_list[i].context_switches));
                        table_OS_Tasks->item(i, 5)->setText(QString::number(task_list[i].runtime));
                        table_OS_Tasks->item(i, 6)->setText(QString::number(task_list[i].stack_size * sizeof(uint32_t)));
                        table_OS_Tasks->item(i, 7)->setText(QString::number(task_list[i].stack_usage * sizeof(uint32_t)));
                    }

                    ++i;
                }

                while (i < l)
                {
                    table_OS_Tasks->removeRow((table_OS_Tasks->rowCount() - 1));
                    ++i;
                }

                table_OS_Tasks->setSortingEnabled(true);
            }
            else if (user_data == ACTION_OS_MCUMGR_BUFFER)
            {
                edit_OS_Info_Output->clear();
                edit_OS_Info_Output->appendPlainText(error_string);
                error_string = nullptr;
            }
            else if (user_data == ACTION_OS_OS_APPLICATION_INFO)
            {
                edit_OS_Info_Output->clear();
                edit_OS_Info_Output->appendPlainText(error_string);
                error_string = nullptr;
            }
            else if (user_data == ACTION_OS_BOOTLOADER_INFO)
            {
                switch (bootloader_info_response.type())
                {
                    case QVariant::Bool:
                    {
                        edit_os_bootloader_response->setText(bootloader_info_response.toBool() == true ? "True" : "False");
                        break;
                    }

                    case QVariant::Int:
                    {
                        edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toInt()));
                        break;
                    }

                    case QVariant::LongLong:
                    {
                        edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toLongLong()));
                        break;
                    }

                    case QVariant::UInt:
                    {
                        edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toUInt()));
                        break;
                    }

                    case QVariant::ULongLong:
                    {
                        edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toULongLong()));
                        break;
                    }

                    case QVariant::Double:
                    {
                        edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toDouble()));
                        break;
                    }

                    case QVariant::String:
                    {
                        edit_os_bootloader_response->setText(bootloader_info_response.toString());
                        break;
                    }

                    default:
                    {
                        error_string = "Invalid";
                    }
                }
            }
        }
    }
    else if (sender() == smp_groups.shell_mgmt)
    {
        qDebug() << "shell sender";
        label_status = lbl_SHELL_Status;

        if (status == STATUS_COMPLETE)
        {
            qDebug() << "complete";
            if (user_data == ACTION_SHELL_EXECUTE)
            {
                if (shell_rc == 0)
                {
                    lbl_SHELL_Status->setText("Finished");
                }
                else
                {
                    lbl_SHELL_Status->setText(QString("Finished, error (ret): ").append(QString::number(shell_rc)));
                }

                edit_SHELL_Output->appendPlainText(error_string);
            }
        }
    }
    else if (sender() == smp_groups.stat_mgmt)
    {
        qDebug() << "stat sender";
        label_status = lbl_STAT_Status;

        if (status == STATUS_COMPLETE)
        {
            qDebug() << "complete";
            if (user_data == ACTION_STAT_GROUP_DATA)
            {
                uint8_t i = 0;

//TODO: optimise
                table_STAT_Values->clearContents();

                while (i < stat_list.length())
                {
                    QTableWidgetItem *row_name = new QTableWidgetItem(stat_list[i].name);
                    QTableWidgetItem *row_value = new QTableWidgetItem(QString::number(stat_list[i].value));

                    table_STAT_Values->insertRow(table_STAT_Values->rowCount());
                    table_STAT_Values->setItem((table_STAT_Values->rowCount() - 1), 0, row_name);
                    table_STAT_Values->setItem((table_STAT_Values->rowCount() - 1), 1, row_value);

                    ++i;
                }
            }
            else if (user_data == ACTION_STAT_LIST_GROUPS)
            {
                combo_STAT_Group->clear();
                combo_STAT_Group->addItems(group_list);
            }
        }
    }
    else if (sender() == smp_groups.fs_mgmt)
    {
        qDebug() << "stat sender";
        label_status = lbl_FS_Status;

        if (status == STATUS_COMPLETE)
        {
            qDebug() << "complete";
            if (user_data == ACTION_FS_UPLOAD)
            {
                edit_FS_Log->appendPlainText("todo");
            }
            else if (user_data == ACTION_FS_DOWNLOAD)
            {
                edit_FS_Log->appendPlainText("todo2");
            }
            else if (user_data == ACTION_FS_HASH_CHECKSUM)
            {
                edit_FS_Log->appendPlainText("todo3");
            }
            else if (user_data == ACTION_FS_SUPPORTED_HASHES_CHECKSUMS)
            {
                uint8_t i = 0;

                combo_FS_Hash_Checksum->clear();

                while (i < supported_hash_checksum_list.length())
                {
                    combo_FS_Hash_Checksum->addItem(supported_hash_checksum_list.at(i).name);
                    qDebug() << supported_hash_checksum_list.at(i).format << ", " << supported_hash_checksum_list.at(i).size;
                    edit_FS_Log->appendPlainText(QString("Has %1, %2, %3").arg(supported_hash_checksum_list[i].name, QString::number(supported_hash_checksum_list[i].format), QString::number(supported_hash_checksum_list[i].size)));
                    ++i;
                }
                edit_FS_Log->appendPlainText("todo4");
            }
            else if (user_data == ACTION_FS_STATUS)
            {
                edit_FS_Log->appendPlainText("todo5");
            }
        }
    }

    if (finished == true)
    {
        mode = ACTION_IDLE;
        relase_transport();

        if (error_string == nullptr)
        {
            if (status == STATUS_COMPLETE)
            {
                error_string = QString("Finished");
            }
            else if (status == STATUS_ERROR)
            {
                error_string = QString("Error");
            }
            else if (status == STATUS_TIMEOUT)
            {
                error_string = QString("Command timed out");
            }
            else if (status == STATUS_CANCELLED)
            {
                error_string = QString("Cancelled");
            }
        }
    }

    if (error_string != nullptr)
    {
        if (label_status != nullptr)
        {
            label_status->setText(error_string);
        }
        else
        {
            qDebug() << "Status message (no receiver): " << error_string;
        }
    }
}

void plugin_mcumgr::progress(uint8_t user_data, uint8_t percent)
{
    Q_UNUSED(user_data);

    if (this->sender() == smp_groups.img_mgmt)
    {
        progress_IMG_Complete->setValue(percent);
    }
    else if (this->sender() == smp_groups.fs_mgmt)
    {
        progress_FS_Complete->setValue(percent);
    }
}

void plugin_mcumgr::group_to_hex(QByteArray *data)
{
    emit plugin_to_hex(data);
}

void plugin_mcumgr::on_btn_transport_connect_clicked()
{
    smp_transport *transport = active_transport();

    if (transport->is_connected() == 1)
    {
        transport->disconnect(true);
    }

    transport->connect();
}

smp_transport *plugin_mcumgr::active_transport()
{
    if (0)
    {
    }
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    else if (radio_transport_udp->isChecked() == true)
    {
        return udp_transport;
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    else if (radio_transport_bluetooth->isChecked() == true)
    {
        return bluetooth_transport;
    }
#endif
    else
    {
        return uart_transport;
    }
}

QMainWindow *plugin_mcumgr::get_main_window()
{
    return parent_window;
}

void plugin_mcumgr::on_radio_transport_uart_toggled(bool checked)
{
    if (checked == true)
    {
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
        udp_transport->close_connect_dialog();
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
        bluetooth_transport->close_connect_dialog();
#endif
    }
}

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
void plugin_mcumgr::on_radio_transport_udp_toggled(bool checked)
{
    if (checked == true)
    {
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
        bluetooth_transport->close_connect_dialog();
#endif
    }
}
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
void plugin_mcumgr::on_radio_transport_bluetooth_toggled(bool checked)
{
    if (checked == true)
    {
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
        udp_transport->close_connect_dialog();
#endif
    }
}
#endif

void plugin_mcumgr::on_radio_OS_Buffer_Info_toggled(bool checked)
{
    if (checked == true)
    {
        edit_OS_UName->setEnabled(false);
    }
}

void plugin_mcumgr::on_radio_OS_uname_toggled(bool checked)
{
    if (checked == true)
    {
        edit_OS_UName->setEnabled(true);
    }
}

void plugin_mcumgr::on_radio_IMG_Get_toggled(bool checked)
{
    if (checked == true)
    {
        check_IMG_Confirm->setEnabled(false);
    }
}

void plugin_mcumgr::on_radio_IMG_Set_toggled(bool checked)
{
    if (checked == true)
    {
        check_IMG_Confirm->setEnabled(true);
    }
}

bool plugin_mcumgr::claim_transport(QLabel *status)
{
    bool successful = false;

    if (active_transport() == uart_transport)
    {
        emit plugin_set_status(true, false, &successful);

        if (successful == false)
        {
            status->setText("Error: Could not claim transport");
        }
    }
    else
    {
        successful = true;
    }

    return successful;
}

void plugin_mcumgr::relase_transport(void)
{
    if (active_transport() == uart_transport)
    {
        bool successful = false;

        emit plugin_set_status(false, false, &successful);

        if (successful == false)
        {
            qDebug() << "Failed to release UART transport";
        }
    }
}
