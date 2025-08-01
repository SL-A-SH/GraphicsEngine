#include "ThemeManager.h"
#include <QStyleFactory>

void ThemeManager::ApplyTheme(QApplication& app, Theme theme)
{
    if (theme == Dark)
    {
        ApplyDarkTheme(app);
    }
    else
    {
        ApplyLightTheme(app);
    }
}

void ThemeManager::ApplyDarkTheme(QApplication& app)
{
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setPalette(CreateDarkPalette());
    app.setStyleSheet(GetDarkStyleSheet());
}

void ThemeManager::ApplyLightTheme(QApplication& app)
{
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setPalette(CreateLightPalette());
    app.setStyleSheet(GetLightStyleSheet());
}

QPalette ThemeManager::CreateDarkPalette()
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Text, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    return darkPalette;
}

QPalette ThemeManager::CreateLightPalette()
{
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ToolTipText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Text, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    lightPalette.setColor(QPalette::Link, QColor(0, 0, 255));
    lightPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    lightPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    return lightPalette;
}

QString ThemeManager::GetDarkStyleSheet()
{
    return R"(
        /* Main Window */
        QMainWindow {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        /* Menu Bar */
        QMenuBar {
            background-color: #2d2d2d;
            color: #ffffff;
            border: none;
        }
        
        QMenuBar::item {
            background-color: transparent;
            padding: 4px 8px;
        }
        
        QMenuBar::item:selected {
            background-color: #404040;
        }
        
        QMenu {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #404040;
        }
        
        QMenu::item {
            padding: 4px 20px;
        }
        
        QMenu::item:selected {
            background-color: #404040;
        }
        
        /* Tool Bar */
        QToolBar {
            background-color: #2d2d2d;
            border: none;
            spacing: 2px;
        }
        
        QToolBar::separator {
            background-color: #404040;
            width: 1px;
            margin: 2px;
        }
        
        /* Dock Widget */
        QDockWidget {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QDockWidget::title {
            background-color: #404040;
            padding: 4px;
            border: 1px solid #404040;
        }
        
        /* Tab Widget */
        QTabWidget::pane {
            border: 1px solid #404040;
            background-color: #2d2d2d;
        }
        
        QTabBar::tab {
            background-color: #404040;
            color: #ffffff;
            padding: 8px 16px;
            margin-right: 2px;
        }
        
        QTabBar::tab:selected {
            background-color: #2596be;
        }
        
        QTabBar::tab:hover {
            background-color: #505050;
        }
        
        /* Buttons */
        QPushButton {
            background-color: #404040;
            color: #ffffff;
            border: 1px solid #505050;
            padding: 5px 15px;
            border-radius: 3px;
        }
        
        QPushButton:hover {
            background-color: #505050;
        }
        
        QPushButton:pressed {
            background-color: #606060;
        }
        
        QPushButton:disabled {
            background-color: #2d2d2d;
            color: #808080;
            border: 1px solid #404040;
        }
        
        /* Combo Box */
        QComboBox {
            background-color: #404040;
            color: #ffffff;
            border: 1px solid #505050;
            padding: 4px;
            border-radius: 3px;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 5px solid #ffffff;
        }
        
        QComboBox QAbstractItemView {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #404040;
            selection-background-color: #404040;
        }
        
        /* Spin Box */
        QSpinBox {
            background-color: #404040;
            color: #ffffff;
            border: 1px solid #505050;
            padding: 4px;
            border-radius: 3px;
        }
        
        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #505050;
            border: none;
            width: 16px;
        }
        
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #606060;
        }
        
        /* Check Box */
        QCheckBox {
            color: #ffffff;
            spacing: 5px;
        }
        
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #505050;
            background-color: #404040;
        }
        
        QCheckBox::indicator:checked {
            background-color: #2a82da;
        }
        
        QCheckBox::indicator:unchecked:hover {
            background-color: #505050;
        }
        
        /* Progress Bar */
        QProgressBar {
            border: 1px solid #404040;
            border-radius: 3px;
            text-align: center;
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QProgressBar::chunk {
            background-color: #2a82da;
            border-radius: 2px;
        }
        
        /* Table Widget */
        QTableWidget {
            background-color: #2d2d2d;
            color: #ffffff;
            gridline-color: #404040;
            border: 1px solid #404040;
        }
        
        QTableWidget::item {
            padding: 5px;
            border: none;
        }
        
        QTableWidget::item:selected {
            background-color: #404040;
        }
        
        QHeaderView::section {
            background-color: #404040;
            color: #ffffff;
            padding: 5px;
            border: 1px solid #505050;
        }
        
        QHeaderView::section:hover {
            background-color: #505050;
        }
        
        /* List Widget */
        QListWidget {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #404040;
        }
        
        QListWidget::item {
            padding: 4px;
            border: none;
        }
        
        QListWidget::item:selected {
            background-color: #404040;
        }
        
        QListWidget::item:hover {
            background-color: #353535;
        }
        
        /* Text Edit */
        QTextEdit {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #404040;
        }
        
        /* Group Box */
        QGroupBox {
            font-weight: bold;
            border: 1px solid #404040;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
            color: #ffffff;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        
        /* Labels */
        QLabel {
            color: #ffffff;
        }
        
        /* Scroll Bars */
        QScrollBar:vertical {
            background-color: #2d2d2d;
            width: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:vertical {
            background-color: #404040;
            border-radius: 6px;
            min-height: 20px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #505050;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        
        QScrollBar:horizontal {
            background-color: #2d2d2d;
            height: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:horizontal {
            background-color: #404040;
            border-radius: 6px;
            min-width: 20px;
        }
        
        QScrollBar::handle:horizontal:hover {
            background-color: #505050;
        }
        
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        
        /* Line Edit */
        QLineEdit {
            background-color: #404040;
            color: #ffffff;
            border: 1px solid #505050;
            padding: 4px;
            border-radius: 3px;
        }
        
        QLineEdit:focus {
            border: 1px solid #2a82da;
        }
        
        /* Slider */
        QSlider::groove:horizontal {
            border: 1px solid #404040;
            height: 8px;
            background-color: #2d2d2d;
            border-radius: 4px;
        }
        
        QSlider::handle:horizontal {
            background-color: #2a82da;
            border: 1px solid #2a82da;
            width: 18px;
            margin: -2px 0;
            border-radius: 9px;
        }
        
        QSlider::handle:horizontal:hover {
            background-color: #3a92ea;
        }
        
        /* Splitter */
        QSplitter::handle {
            background-color: #404040;
        }
        
        QSplitter::handle:horizontal {
            width: 1px;
        }
        
        QSplitter::handle:vertical {
            height: 1px;
        }
        
        /* Status Bar */
        QStatusBar {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QStatusBar::item {
            border: none;
        }
        
        /* Tool Tips */
        QToolTip {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #404040;
            padding: 4px;
        }
    )";
}

QString ThemeManager::GetLightStyleSheet()
{
    return R"(
        /* Light theme - minimal styling to use system defaults */
        QMainWindow {
            background-color: #f0f0f0;
        }
        
        QMenuBar {
            background-color: #f0f0f0;
        }
        
        QToolBar {
            background-color: #f0f0f0;
            border: none;
        }
        
        QDockWidget {
            background-color: #f0f0f0;
        }
        
        QTabWidget::pane {
            border: 1px solid #c0c0c0;
            background-color: #f0f0f0;
        }
        
        QTabBar::tab {
            background-color: #e0e0e0;
            padding: 8px 16px;
            margin-right: 2px;
        }
        
        QTabBar::tab:selected {
            background-color: #f0f0f0;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 1px solid #c0c0c0;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
    )";
} 