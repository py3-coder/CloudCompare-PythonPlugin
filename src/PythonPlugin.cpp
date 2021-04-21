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
#include "PrivateRuntime.h"
#include "PythonPlugin.h"
#include "PythonStdErrOutRedirect.h"
#include "QPythonEditor.h"
#include "QPythonREPL.h"
#include "FileRunner.h"
#include "Utilities.h"

#include <QUrl>
#include <QDesktopServices>

#include <memory>

#define slots Q_SLOTS
#define signals Q_SIGNALS
#include <ccCommandLineInterface.h>

void logPythonPath()
{
    const wchar_t *pythonPath = Py_GetPath();
    if (pythonPath != nullptr)
    {
        size_t errPos{0};
        char *cPythonPath = Py_EncodeLocale(pythonPath, &errPos);
        if (cPythonPath)
        {
            ccLog::Print("[PythonPlugin] PythonPath is set to: %s", cPythonPath);
        }
        else
        {
            ccLog::Print("[PythonPlugin] Failed to convert the PythonPath");
        }
    }
    else
    {
        ccLog::Print("[PythonPlugin] PythonPath is not set");
    }
}

void logPythonHome()
{
    const wchar_t *pythonHome = Py_GetPythonHome();
    if (pythonHome != nullptr)
    {
        size_t errPos{0};
        char *cPythonHome = Py_EncodeLocale(pythonHome, &errPos);
        if (cPythonHome)
        {
            ccLog::Print("[PythonPlugin] PythonHome is set to: %s", cPythonHome);
            PyMem_Free(cPythonHome);
        }
        else
        {
            ccLog::Print("[PythonPlugin]Failed to convert the PythonHome path");
        }
    }
    else
    {
        ccLog::Print("[PythonPlugin] PythonHome is not set");
    }
}

// Useful link:
// https://docs.python.org/3/c-api/init.html#initialization-finalization-and-threads
PythonPlugin::PythonPlugin(QObject *parent)
    : QObject(parent), ccStdPluginInterface(":/CC/plugin/PythonPlugin/info.json"),
     m_interp(nullptr),  m_editor(new QPythonEditor(&m_interp))
{
    m_interp.initialize();

    logPythonHome();
    logPythonPath();

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &PythonPlugin::finalizeInterpreter);
}

QList<QAction *> PythonPlugin::getActions()
{
    bool enableActions = PythonInterpreter::isInitialized();

    if (!m_showEditor)
    {
        m_showEditor = new QAction("Show Editor", this);
        m_showEditor->setToolTip("Show the code editor window");
        m_showEditor->setIcon(QIcon(":/CC/plugin/PythonPlugin/images/python-editor-icon.png"));
        connect(m_showEditor, &QAction::triggered, this, &PythonPlugin::showEditor);
        m_showEditor->setEnabled(enableActions);
    }

    if (!m_showREPL)
    {
        m_showREPL = new QAction("Show REPL", this);
        m_showREPL->setToolTip("Show the Python REPL");
        m_showREPL->setIcon(QIcon(":/CC/plugin/PythonPlugin/images/repl-icon.png"));
        connect(m_showREPL, &QAction::triggered, this, &PythonPlugin::showRepl);
        m_showREPL->setEnabled(enableActions);
    }

    if (!m_showDoc)
    {
        m_showDoc = new QAction("Show Documentation", this);
        m_showDoc->setToolTip("Show local documentation in your web browser");
        m_showDoc->setIcon(m_app->getMainWindow()->style()->standardIcon(QStyle::SP_FileDialogInfoView));
        connect(m_showDoc, &QAction::triggered, &PythonPlugin::showDocumentation);
        m_showDoc->setEnabled(enableActions);
    }

    if (!m_showAboutDialog)
    {
        m_showAboutDialog = new QAction("About", this);
        m_showAboutDialog->setToolTip("About this plugin");
        m_showAboutDialog->setIcon(
            m_app->getMainWindow()->style()->standardIcon(QStyle::SP_MessageBoxQuestion));
        connect(m_showAboutDialog, &QAction::triggered, this, &PythonPlugin::showAboutDialog);
        m_showAboutDialog->setEnabled(enableActions);
    }

    if (!m_showFileRunner)
    {
        m_showFileRunner = new QAction("File Runner", this);
        m_showFileRunner->setToolTip("Small widget to select and run a script");
        m_showFileRunner->setIcon(QIcon(":/CC/plugin/PythonPlugin/images/runner-icon.png"));
        connect(m_showFileRunner, &QAction::triggered, this, &PythonPlugin::showFileRunner);
        m_showFileRunner->setEnabled(enableActions);
    }

    return { m_showEditor, m_showFileRunner, m_showAboutDialog, m_showDoc, m_showREPL };
}

void PythonPlugin::showRepl()
{
    if (m_repl)
    {
        m_repl->show();
        m_repl->raise();
        m_repl->activateWindow();
    }
    else
    {
        m_repl = new QPythonREPL(&m_interp);
        m_repl->show();
    }
}

void PythonPlugin::showEditor() const
{
    if (m_editor)
    {
        m_editor->show();
        m_editor->raise();
        m_editor->activateWindow();
    }
}

void PythonPlugin::showFileRunner() const {
    m_fileRunner->show();
}

void PythonPlugin::showDocumentation()
{
	const QUrl url(QString("https://tmontaigu.github.io/CloudCompare-PythonPlugin/index.html"));
	QDesktopServices::openUrl(url);
}

void PythonPlugin::showAboutDialog() const
{
    AboutDialog dlg(m_app->getMainWindow());
    dlg.exec();
}

PythonPlugin::~PythonPlugin()
{
    Python::unsetMainAppInterfaceInstance();
    Python::unsetCmdLineInterfaceInstance();
}

struct PythonPluginCommand : public ccCommandLineInterface::Command
{
    explicit PythonPluginCommand(PythonInterpreter *interpreter_)
        : Command("PYTHON", "PYTHON_SCRIPT"), interpreter(interpreter_)
    {
    }

    bool process(ccCommandLineInterface &cmd) override
    {
        Q_ASSERT(interpreter);
        cmd.print("[PythonPlugin] Starting");
        Args args;
        if (!args.parseFrom(cmd))
        {
            return false;
        }

        PySys_SetArgvEx(static_cast<int>(args.pythonArgv.size()), args.pythonArgv.data(), 1);
        bool success = interpreter->executeFile(qPrintable(args.filepath));

        cmd.print(
            QString("[PythonPlugin] Script %1 executed").arg(success ? "successfully" : "unsuccessfully"));
        return success;
    }

    struct Args
    {
        QString filepath;
        std::vector<wchar_t *> pythonArgv{};

        Args() = default;

        bool parseFrom(ccCommandLineInterface &cmd)
        {
            if (cmd.arguments().empty())
            {
                return cmd.error(
                    QString("Missing parameter: parameters filename after \"-%1\"").arg("PYTHON_SCRIPT"));
            }
            filepath = cmd.arguments().takeFirst();

            pythonArgv.reserve(cmd.arguments().size() + 1);
            pythonArgv.push_back(QStringToWcharArray(filepath));
            while (!cmd.arguments().isEmpty())
            {
                pythonArgv.push_back(QStringToWcharArray(cmd.arguments().takeFirst()));
            }
            return true;
        }

        virtual ~Args()
        {
            for (wchar_t *arg : pythonArgv)
            {
                delete[] arg;
            }
        }
    };

    PythonInterpreter *interpreter{nullptr};
};

void PythonPlugin::registerCommands(ccCommandLineInterface *cmd)
{
    cmd->registerCommand(ccCommandLineInterface::Command::Shared(new PythonPluginCommand(&m_interp)));
    Python::setCmdLineInterfaceInstance(cmd);
}

void PythonPlugin::setMainAppInterface(ccMainAppInterface *app)
{
    ccStdPluginInterface::setMainAppInterface(app);
    Python::setMainAppInterfaceInstance(m_app);
    m_fileRunner = new FileRunner(&m_interp, m_app->getMainWindow());
}

void PythonPlugin::finalizeInterpreter()
{
    m_interp.finalize();
}
