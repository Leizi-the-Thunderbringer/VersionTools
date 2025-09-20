#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QDir>
#include "VersionToolsMainWindow.h"
#include "utils/ThemeManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Version Tools");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("VersionTools Project");
    app.setOrganizationDomain("versiontools.dev");
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/versiontools.png"));
    
    // Apply modern theme
    ThemeManager::applyModernTheme(&app);
    
    // Create and show main window
    VersionToolsMainWindow window;
    window.show();
    
    return app.exec();
}