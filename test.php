<?PHP
//Copyright (C) 2023 Jamie M.
//Take built ui form file and split into .h and .cpp file for AuTerm plugins
$File = 'ui_form.h';

//Text to find, text to add/replace, 0 = replace | 1 = prepend (start of line) | 2 = append (to next line)
$Replacement1 = array(
	array('QGridLayout *gridLayout;', '//', 1),
	array('QTabWidget *tabWidget;', '//', 1),
);

$Replacement2 = array(
	array('gridLayout = new QGridLayout(', '//', 1),
	array('gridLayout->setObjectName(QString::fromUtf8("gridLayout"));', '//', 1),
	array('tabWidget = new QTabWidget(', '//', 1),
	array('tabWidget->setObjectName(QString::fromUtf8("tabWidget"));', '//', 1),
	array('tab = new QWidget();', 'tab = new QWidget(tabWidget_orig);', 0),
	array('verticalLayoutWidget = new QWidget(tab);', 'verticalLayoutWidget = new QWidget();', 0),
	array('verticalLayoutWidget->setGeometry(QRect(410, 20, 229, 182));', 'verticalLayoutWidget->setGeometry(QRect(6, 6, 229, 182));', 0),
	array('tabWidget->addTab(tab, QString());', '//', 1),
	array('gridLayout->addWidget(tabWidget, 0, 0, 1, 1);', '//', 1),
	array('retranslateUi(Form);', '//', 1),
	array('tabWidget->setCurrentIndex(0);', '//', 1),
	array('QMetaObject::connectSlotsByName(Form);', '//', 1),
);

$Replacement3 = array(
	array('Form->setWindowTitle(QCoreApplication::translate("Form", "Form", nullptr));', '//', 1),
	array('tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("Form", "MCUmgr", nullptr));', '//', 1),
);

//Start text, end text, number of lines to remove from start, number of lines to remove from end, replace array, file, block comment start/end
$Params = array(
	array('#include', 'QT_BEGIN_NAMESPACE', 0, 1, null, 'plugin_mcumgr.h', 'INCLUDES'),
	array('public:', 'QWidget *Form', 1, 1, $Replacement1, 'plugin_mcumgr.h', 'OBJECTS'),
	array('Form->resize', 'QMetaObject::connectSlotsByName', 1, 2, $Replacement2, 'plugin_mcumgr.cpp', 'INIT'),
	array('void retranslateUi', '// retranslateUi', 2, 0, $Replacement3, 'plugin_mcumgr.cpp', 'TRANSLATE'),
);

$Dat = explode("\n", file_get_contents($File));
$CurrentSet = 0;
$CurrentSetFound = false;
$Output = array();
$SkipLines = 0;
$Finished = false;

foreach ($Dat as $ThisLine)
{
	if ($CurrentSetFound == false && $Finished == false)
	{
		if (strpos($ThisLine, $Params[$CurrentSet][0]) !== false)
		{
			$CurrentSetFound = true;
			$SkipLines = $Params[$CurrentSet][2];
		}
	}

	if ($CurrentSetFound == true)
	{
		if (strpos($ThisLine, $Params[$CurrentSet][1]) !== false)
		{
			$SkipLines = $Params[$CurrentSet][3];

			while ($SkipLines > 0)
			{
				unset($Output[$CurrentSet][(count($Output[$CurrentSet])-1)]);
				--$SkipLines;
			}

			$CurrentSetFound = false;
			++$CurrentSet;

			if ($CurrentSet >= count($Params))
			{
				$Finished = true;
			}
		}
	}

	if ($CurrentSetFound == true)
	{
		if ($SkipLines > 0)
		{
			--$SkipLines;
		}
		else
		{
			if (substr($ThisLine, 0, 8) == '        ')
			{
				//Reduce excess spacing
				$ThisLine = substr($ThisLine, 4);
			}

			if (!is_null($Params[$CurrentSet][4]))
			{
				$ReplacementData = $Params[$CurrentSet][4];
				foreach ($ReplacementData as $ThisReplacement)
				{
					if (strpos($ThisLine, $ThisReplacement[0]) !== false)
					{
						if ($ThisReplacement[2] == 0)
						{
							//Replace
							$ThisLine = str_replace($ThisReplacement[0], $ThisReplacement[1], $ThisLine);
						}
						else if ($ThisReplacement[2] == 1)
						{
							//Prepend
							$ThisLine = $ThisReplacement[1].$ThisLine;
						}
						else if ($ThisReplacement[2] == 2)
						{
							//Append
							$ThisLine .= "\n".$ThisReplacement[1];
						}
					}
				}
			}

			$Output[$CurrentSet][] = $ThisLine;
		}
	}
}

foreach ($Params as $ID => $ThisParams)
{
	$SearchStartString = '///AUTOGEN_START_'.$ThisParams[6];
	$SearchEndString = '///AUTOGEN_END_'.$ThisParams[6];
	$Found = false;
	$FileData = explode("\n", file_get_contents('plugins/mcumgr/'.$ThisParams[5]));

	foreach ($FileData as $ThisIndex => $ThisLine)
	{
		if (strpos($ThisLine, $SearchStartString) !== false)
		{
			$Found = true;
		}
		else if (strpos($ThisLine, $SearchEndString) !== false)
		{
			$FileData[$ThisIndex] = implode("\n", $Output[$ID])."\n".$FileData[$ThisIndex];
			break;
		}
		else if ($Found == true)
		{
			unset($FileData[$ThisIndex]);
		}
	}

	file_put_contents('plugins/mcumgr/'.$ThisParams[5], implode("\n", $FileData));
}
?>
