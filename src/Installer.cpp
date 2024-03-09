#include "Installer.h"

#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <iostream>

Installer::Installer(QObject* parent)
    : QObject(parent)
    , m_pkgman_path(QDir::homePath() + QStringLiteral("/RetroPie-Setup/retropie_packages.sh"))
    , m_task_running(false)
    , m_task_failed(false)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    // m_process.setWorkingDirectory(QStringLiteral("/home/pi"));
    connect(&m_process, &QProcess::readyRead, this, &Installer::onProcessReadyRead);
    connect(&m_process, &QProcess::errorOccurred, this, &Installer::onProcessError);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Installer::onProcessFinished);
}

bool Installer::retropieAvailable() const
{
    const QFileInfo pkgman(m_pkgman_path);
    bool exists = pkgman.exists();
    bool isFile = pkgman.isFile();
    bool isExecutable = pkgman.isExecutable();

    // Open a log file in append mode
    QFile logFile("/home/pi/fe_debug_log.txt"); // Specify the path to your log file
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream logStream(&logFile);
        logStream << "Checking RetroPie Availability:\n";
        logStream << "Path: " << m_pkgman_path << "\n";
        logStream << "Exists: " << (exists ? "Yes" : "No") << "\n";
        logStream << "Is File: " << (isFile ? "Yes" : "No") << "\n";
        logStream << "Is Executable: " << (isExecutable ? "Yes" : "No") << "\n\n";
        logFile.flush(); // Ensures that all data is written to the file
    }

    return exists && isFile && isExecutable;
}


bool Installer::installed(Frontend* frontend) const
{
    Q_ASSERT(frontend);

    const QString path = frontend->m_exe_path.startsWith('/')
        ? frontend->m_exe_path
        : QStringLiteral("/usr/bin/") + frontend->m_exe_path;

    return QFileInfo::exists(path);
}

void Installer::startInstall(Frontend* frontend)
{
    Q_ASSERT(frontend);
    Q_ASSERT(m_process.state() != QProcess::Running);

    const QStringList arguments {
        frontend->m_package_name,
        QStringLiteral("install")
    };

    m_log = QStringLiteral("Launching `") + m_pkgman_path;
    for (const QString& arg : arguments)
        m_log += ' ' + arg;
    m_log += "`...\n";
    emit logChanged();

    m_task_running = true;
    m_task_failed = false;
    emit taskRunChanged();
    emit taskFailChanged();

    m_process.start(m_pkgman_path, arguments, QIODevice::ReadOnly);
}

void Installer::onProcessReadyRead()
{
    const auto prev_log_len = m_log.length();

    m_log += QString(m_process.readAllStandardOutput());

    if (prev_log_len != m_log.length())
        emit logChanged();
}

void Installer::onProcessError(QProcess::ProcessError)
{
    if (!m_log.isEmpty())
        m_log += '\n';

    m_log += m_process.errorString();
    emit logChanged();

    m_task_running = false;
    m_task_failed = true;
    emit taskRunChanged();
    emit taskFailChanged();
}

void Installer::onProcessFinished(int, QProcess::ExitStatus)
{
    m_task_running = false;
    m_task_failed = false;
    emit taskRunChanged();
    emit taskFailChanged();
}
