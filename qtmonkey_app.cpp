#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QTextStream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "common.hpp"
#include "qtmonkey.hpp"

namespace {
    class ConsoleApplication final : public QCoreApplication {
    public:
        ConsoleApplication(int &argc, char **argv): QCoreApplication(argc, argv) {}
        bool notify(QObject *receiver, QEvent *event) override {
            try {
                return QCoreApplication::notify(receiver, event);
            } catch (const std::exception &ex) {
                qFatal("%s: catch exception: %s", Q_FUNC_INFO, ex.what());
                return false;
            }
        }
    };
}

#if QT_VERSION >= 0x050000
static void msgHandler(QtMsgType type, const QMessageLogContext &,
                       const QString &msg)
#else
static void msgHandler(QtMsgType type, const char *msg)
#endif
{
    QTextStream clog(stderr);
    switch (type) {
    case QtDebugMsg:
        clog << "Debug: " << msg << "\n";
        break;
    case QtWarningMsg:
        clog << "Warning: " << msg << "\n";
        break;
    case QtCriticalMsg:
        clog << "Critical: " << msg << "\n";
        break;
#if QT_VERSION >= 0x050000
    case QtInfoMsg:
        clog << "Info: " << msg << "\n";
        break;
#endif
    case QtFatalMsg:
        clog << "Fatal: " << msg << "\n";
        clog.flush();
        std::abort();
    }
    clog.flush();
}

static QString usage()
{
    return T_("Usage: %1 [--script path/to/script] --user-app "
              "path/to/application [application's command line args]\n")
        .arg(QCoreApplication::applicationFilePath());
}

int main(int argc, char *argv[])
{
    ConsoleApplication app(argc, argv);

    INSTALL_QT_MSG_HANDLER(msgHandler);

    QTextStream cout(stdout);
    QTextStream cerr(stderr);

    int userAppOffset = -1;    
    QStringList scripts;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--user-app") == 0) {
            if ((i + 1) >= argc) {
                cerr << usage();
                return EXIT_FAILURE;
            }
            ++i;
            userAppOffset = i;
            break;
        } else if (std::strcmp(argv[i], "--script") == 0) {
            if ((i + 1) >= argc) {
                cerr << usage();
                return EXIT_FAILURE;
            }
            ++i;
            scripts.append(QFile::decodeName(argv[i]));
        } else {
            cerr << T_("Unknown option: %1\n").arg(argv[i]);
            cerr << usage();
            return EXIT_FAILURE;
        }
    if (userAppOffset == -1) {
        cerr << T_(
            "You should set path and args for user app with --user-app\n");
        return EXIT_FAILURE;
    }
    QStringList userAppArgs;
    for (int i = userAppOffset + 1; i < argc; ++i)
        userAppArgs << QString::fromLocal8Bit(argv[i]);
    qt_monkey_app::QtMonkey monkey;

    if (!scripts.empty() && !monkey.runScriptFromFile(std::move(scripts)))
        return EXIT_FAILURE;

    monkey.runApp(QString::fromLocal8Bit(argv[userAppOffset]),
                  std::move(userAppArgs));

    return app.exec();
}
