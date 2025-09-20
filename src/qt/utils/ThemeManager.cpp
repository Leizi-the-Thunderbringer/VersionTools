#include "ThemeManager.h"
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>

void ThemeManager::applyModernTheme(QApplication *app)
{
#ifdef Q_OS_WIN
    // Use Fusion style on Windows for better custom styling
    app->setStyle(QStyleFactory::create("Fusion"));
#elif defined(Q_OS_LINUX)
    // Use Fusion style on Linux as well
    app->setStyle(QStyleFactory::create("Fusion"));
#endif

    QPalette palette;
    setModernPalette(palette);
    app->setPalette(palette);
    
    app->setStyleSheet(getModernStyleSheet());
}

void ThemeManager::applyDarkTheme(QApplication *app)
{
    app->setStyle(QStyleFactory::create("Fusion"));
    
    QPalette palette;
    setDarkPalette(palette);
    app->setPalette(palette);
}

void ThemeManager::applyLightTheme(QApplication *app)
{
    app->setStyle(QStyleFactory::create("Fusion"));
    
    QPalette palette;
    setLightPalette(palette);
    app->setPalette(palette);
}

void ThemeManager::setModernPalette(QPalette &palette)
{
    // Modern light theme with subtle colors
    palette.setColor(QPalette::Window, QColor(248, 249, 250));
    palette.setColor(QPalette::WindowText, QColor(33, 37, 41));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(248, 249, 250));
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipText, QColor(33, 37, 41));
    palette.setColor(QPalette::Text, QColor(33, 37, 41));
    palette.setColor(QPalette::Button, QColor(248, 249, 250));
    palette.setColor(QPalette::ButtonText, QColor(33, 37, 41));
    palette.setColor(QPalette::BrightText, QColor(220, 53, 69));
    palette.setColor(QPalette::Link, QColor(0, 120, 215));
    palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    
    // Disabled colors
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(108, 117, 125));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(108, 117, 125));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(108, 117, 125));
}

void ThemeManager::setDarkPalette(QPalette &palette)
{
    // Dark theme
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, QColor(0, 0, 0));
    palette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    palette.setColor(QPalette::Text, QColor(255, 255, 255));
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    palette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
}

void ThemeManager::setLightPalette(QPalette &palette)
{
    // Light theme - use system default with some modifications
    palette = QApplication::style()->standardPalette();
    palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    palette.setColor(QPalette::Link, QColor(0, 120, 215));
}

QString ThemeManager::getModernStyleSheet()
{
    return R"(
        QMainWindow {
            background-color: #f8f9fa;
        }
        
        QMenuBar {
            background-color: #ffffff;
            border-bottom: 1px solid #dee2e6;
            padding: 4px;
        }
        
        QMenuBar::item {
            padding: 6px 12px;
            border-radius: 4px;
            margin: 2px;
        }
        
        QMenuBar::item:selected {
            background-color: #e9ecef;
        }
        
        QMenuBar::item:pressed {
            background-color: #dee2e6;
        }
        
        QMenu {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            padding: 4px;
        }
        
        QMenu::item {
            padding: 8px 16px;
            border-radius: 4px;
            margin: 1px;
        }
        
        QMenu::item:selected {
            background-color: #0078d4;
            color: white;
        }
        
        QMenu::separator {
            height: 1px;
            background-color: #dee2e6;
            margin: 4px 8px;
        }
        
        QToolBar {
            background-color: #ffffff;
            border: none;
            border-bottom: 1px solid #dee2e6;
            padding: 4px;
            spacing: 4px;
        }
        
        QToolButton {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 6px;
            margin: 2px;
        }
        
        QToolButton:hover {
            background-color: #e9ecef;
            border-color: #adb5bd;
        }
        
        QToolButton:pressed {
            background-color: #dee2e6;
        }
        
        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #dee2e6;
            padding: 4px;
        }
        
        QPushButton {
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 500;
        }
        
        QPushButton:hover {
            background-color: #106ebe;
        }
        
        QPushButton:pressed {
            background-color: #005a9e;
        }
        
        QPushButton:disabled {
            background-color: #e9ecef;
            color: #6c757d;
        }
        
        QPushButton[class="secondary"] {
            background-color: #6c757d;
        }
        
        QPushButton[class="secondary"]:hover {
            background-color: #5c636a;
        }
        
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: #ffffff;
            border: 1px solid #ced4da;
            border-radius: 6px;
            padding: 8px 12px;
            selection-background-color: #0078d4;
        }
        
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #0078d4;
            outline: none;
        }
        
        QListWidget, QTreeWidget, QTableWidget {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            alternate-background-color: #f8f9fa;
        }
        
        QListWidget::item, QTreeWidget::item, QTableWidget::item {
            padding: 8px;
            border-radius: 4px;
            margin: 1px;
        }
        
        QListWidget::item:selected, QTreeWidget::item:selected, QTableWidget::item:selected {
            background-color: #0078d4;
            color: white;
        }
        
        QListWidget::item:hover, QTreeWidget::item:hover, QTableWidget::item:hover {
            background-color: #e9ecef;
        }
        
        QScrollBar:vertical {
            background-color: #f8f9fa;
            width: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:vertical {
            background-color: #adb5bd;
            border-radius: 6px;
            min-height: 20px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #6c757d;
        }
        
        QScrollBar:horizontal {
            background-color: #f8f9fa;
            height: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:horizontal {
            background-color: #adb5bd;
            border-radius: 6px;
            min-width: 20px;
        }
        
        QScrollBar::handle:horizontal:hover {
            background-color: #6c757d;
        }
        
        QScrollBar::add-line, QScrollBar::sub-line {
            border: none;
            background: none;
        }
        
        QSplitter::handle {
            background-color: #dee2e6;
        }
        
        QSplitter::handle:horizontal {
            width: 3px;
        }
        
        QSplitter::handle:vertical {
            height: 3px;
        }
        
        QSplitter::handle:hover {
            background-color: #0078d4;
        }
        
        QTabWidget::pane {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 6px;
        }
        
        QTabBar::tab {
            background-color: #f8f9fa;
            border: 1px solid #dee2e6;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            padding: 8px 16px;
            margin-right: 2px;
        }
        
        QTabBar::tab:selected {
            background-color: #ffffff;
            border-bottom: 1px solid #ffffff;
        }
        
        QTabBar::tab:hover {
            background-color: #e9ecef;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 8px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 8px 0 8px;
        }
        
        QComboBox {
            background-color: #ffffff;
            border: 1px solid #ced4da;
            border-radius: 6px;
            padding: 6px 12px;
            min-width: 6em;
        }
        
        QComboBox:hover {
            border-color: #0078d4;
        }
        
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left: 1px solid #ced4da;
            border-top-right-radius: 6px;
            border-bottom-right-radius: 6px;
        }
        
        QComboBox::down-arrow {
            image: url(:/icons/chevron-down.svg);
            width: 12px;
            height: 12px;
        }
        
        QComboBox QAbstractItemView {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            selection-background-color: #0078d4;
        }
        
        QProgressBar {
            background-color: #e9ecef;
            border: none;
            border-radius: 6px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 6px;
        }
    )";
}