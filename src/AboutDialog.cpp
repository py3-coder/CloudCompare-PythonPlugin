//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: PythonPlugin                       #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                   COPYRIGHT: Thomas Montaigu                           #
//#                                                                        #
//##########################################################################

#include "AboutDialog.h"

#include <ui_AboutDialog.h>

#include <pybind11/pybind11.h>


AboutDialog::AboutDialog(QWidget *parent): QDialog(parent), m_dlg(new Ui_AboutDialog) {
    m_dlg->setupUi(this);

    QObject::connect(m_dlg->okBtn, &QPushButton::clicked, this, &QDialog::close);

    const char* versionStr = Py_GetVersion();
    m_dlg->pythonVersionLabel->setText(QString("<p align=\"center\"> %1 </p>").arg(versionStr));
}
