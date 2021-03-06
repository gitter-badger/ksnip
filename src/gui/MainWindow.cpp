/*
 *  Copyright (C) 2016 Damir Porobic <https://github.com/damirporobic>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "MainWindow.h"

MainWindow::MainWindow(AbstractImageGrabber *imageGrabber, RunMode mode)
    :
    QMainWindow(),
    mImageGrabber(imageGrabber),
    mMode(mode),
    mSaveButton(new QToolButton(this)),
    mCopyToClipboardButton(new QToolButton(this)),
    mSettingsButton(new SettingsPicker(this, 5)),
    mSaveAction(new QAction(this)),
    mCopyToClipboardAction(new QAction(this)),
    mUploadToImgurAction(new QAction(this)),
    mPrintAction(new QAction(this)),
    mPrintPreviewAction(new QAction(this)),
    mCropAction(new QAction(this)),
    mNewCaptureAction(new QAction(this)),
    mQuitAction(new QAction(this)),
    mSettingsDialogAction(new QAction(this)),
    mAboutKsnipAction(new QAction(this)),
    mOpenImageAction(new QAction(this)),
    mScaleAction(new QAction(this)),
    mPaintArea(new PaintArea()),
    mCaptureView(new CaptureView(mPaintArea)),
    mUndoAction(mPaintArea->getUndoAction()),
    mRedoAction(mPaintArea->getRedoAction()),
    mClipboard(QApplication::clipboard()),
    mCropPanel(new CropPanel(mCaptureView)),
    mConfig(KsnipConfig::instance()),
    mSettingsPickerConfigurator(new SettingsPickerConfigurator()),
    mDelayHandler(new DelayHandler(200)),
    mToolPicker(new ToolPicker()),
    mCaptureModePicker(new CaptureModePicker(imageGrabber->supportedCaptureModes())),
    mCapturePrinter(new CapturePrinter(mPaintArea)),
    mCaptureUploader(new CaptureUploader())
{
    // When we run in CLI only mode we don't need to setup gui, but only need
    // to connect imagegrabber signals to mainwindow slots to handle the
    // feedback.
    if (mMode == RunMode::CLI) {
        connect(mImageGrabber, &AbstractImageGrabber::finished, [this](const QPixmap &pixmap) {
            instantSave(pixmap);
            close();
        });
        connect(mImageGrabber, &AbstractImageGrabber::canceled, this, &MainWindow::close);
        return;
    }

    initGui();

    mCaptureView->hide();

    setWindowIcon(IconLoader::loadIcon(QStringLiteral("ksnip")));
    move(mConfig->windowPosition());

    connect(mPaintArea, &PaintArea::imageChanged, this, &MainWindow::screenshotChanged);
    connect(mCaptureView, &CaptureView::imageCropped, this, &MainWindow::screenshotChanged);

    connect(mImageGrabber, &AbstractImageGrabber::finished, this, &MainWindow::showCapture);
    connect(mImageGrabber, &AbstractImageGrabber::canceled, [this]() { setHidden(false); });

    connect(mCaptureUploader, &CaptureUploader::finished, this, &MainWindow::uploadFinished);

    connect(mCropPanel, &CropPanel::closing, this, &MainWindow::closeCrop);
    connect(mCaptureView, &CaptureView::closeCrop, this, &MainWindow::closeCrop);

    loadSettings();

    if (mMode == RunMode::GUI) {
        if (mConfig->captureOnStartup()) {
            capture(mConfig->captureMode());
        } else {
            show();
        }
    }
}

void MainWindow::screenshotChanged()
{
    setSaveAble(true);

    if (mConfig->alwaysCopyToClipboard()) {
            copyToClipboard();
    }
}

//
// Public Functions
//

void MainWindow::captureScreenshot(CaptureModes captureMode, bool captureCursor, int delay)
{
    delay = mDelayHandler->getDelay(delay);
    mImageGrabber->grabImage(captureMode, captureCursor, delay);
}

/*
 * Sets the Main Window size to fit all content correctly, it takes into account
 * if an image was loaded or not,  if the status bar is show or not, and so on.
 * If no image is loaded the status bar
 * is hidden too.
 */
void MainWindow::resize()
{
    if (!mPaintArea->isValid()) {
        statusBar()->setHidden(true);
        QWidget::resize(minimumSize());
    } else {
        statusBar()->setHidden(false);
        mPaintArea->fitViewToParent();
    }
}

void MainWindow::showCapture(const QPixmap& screenshot)
{
    if (screenshot.isNull()) {
        qCritical("PaintWindow::showWindow: No image provided to but it was expected.");
        return show();
    }

    setHidden(false);
    mPaintArea->loadCapture(screenshot);
    mPaintArea->setIsEnabled(true);

    if (mPaintArea->areaSize().width() > mImageGrabber->currectScreenRect().width() ||
            mPaintArea->areaSize().height() > mImageGrabber->currectScreenRect().height()) {
        setWindowState(Qt::WindowMaximized);
    } else {
        resize();
    }

    setSaveAble(true);
    setEnablements(true);
    closeCrop();

    if (mConfig->alwaysCopyToClipboard()) {
        copyToClipboard();
    }

    mCaptureView->show();
    QMainWindow::show();
}

void MainWindow::show()
{
    setHidden(false);
    mCaptureView->hide();
    setSaveAble(false);
    setEnablements(false);
    closeCrop();
    QMainWindow::show();
}

//
// Public Slots
//
void MainWindow::openCrop()
{
    if (!mPaintArea->isValid()) {
        return;
    }
    statusBar()->addPermanentWidget(mCropPanel, 1);
    mCropPanel->show();
    statusBar()->setHidden(false);
}

void MainWindow::closeCrop()
{
    statusBar()->removeWidget(mCropPanel);
    statusBar()->setHidden(true);
}

QMenu* MainWindow::createPopupMenu()
{
    // Filtering out the option to hide main toolbar which should no be allowed.
    auto filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(mToolBar->toggleViewAction());
    return filteredMenu;
}

void MainWindow::colorChanged(const QColor& color)
{
    mConfig->setToolColor(mPaintArea->paintMode(), color);
}

void MainWindow::fillChanged(bool fill)
{
    mConfig->setToolFill(mPaintArea->paintMode(), fill);
}

void MainWindow::sizeChanged(int size)
{
    mConfig->setToolSize(mPaintArea->paintMode(), size);
}

//
// Protected Functions
//

void MainWindow::moveEvent(QMoveEvent* event)
{
    mConfig->setWindowPosition(pos());
    QWidget::moveEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    auto canDiscard = discardUnsavedChanges();
    if (!canDiscard) {
        saveCapture();
        event->ignore();
        return;
    }
    event->accept();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Shortcut && mPaintArea->isTextEditing()) {
        auto shortcutEvent = dynamic_cast<QShortcutEvent *>(event);
        auto s = shortcutEvent->key().toString().toLower();

        // If Ctrl or Alt was pressed we can ignore those events as they
        // don't result in printable characters. Other shortcuts are
        // currently not used.
        if (s.contains(QStringLiteral("ctrl")) || s.contains(QStringLiteral("alt"))) {
            return true;
        }

        // If the string contains shift, upper case was requested, we remove
        // the shift and + character and make the character uppercase
        if (s.contains(QStringLiteral("shift"))) {
            s = s.remove(QStringLiteral("shift+")).toUpper();
        }

        // Create new event and send it to the focused item.
        auto myEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, s);
        QCoreApplication::postEvent(QApplication::focusWidget(), myEvent);
        return true;
    }
    return QObject::eventFilter(watched, event);
}

//
// Private Functions
//

void MainWindow::setSaveAble(bool enabled)
{
    if (enabled) {
        setWindowTitle(QStringLiteral("*") + QApplication::applicationName() + " - " + tr("Unsaved"));
    } else {
        setWindowTitle(QApplication::applicationName());
    }
    mSaveAction->setEnabled(enabled);
    mIsUnsaved = enabled;
}

void MainWindow::setEnablements(bool enabled)
{
    mCropAction->setEnabled(enabled);
    mPrintAction->setEnabled(enabled);
    mPrintPreviewAction->setEnabled(enabled);
    mUploadToImgurAction->setEnabled(enabled);
    mCopyToClipboardAction->setEnabled(enabled);
    mScaleAction->setEnabled(enabled);
}

void MainWindow::loadSettings()
{
    auto paintMode = mConfig->paintMode();
    mToolPicker->setTool(paintMode);
    setPaintMode(paintMode);

    mCaptureModePicker->setCaptureMode(mConfig->captureMode());
}

void MainWindow::copyToClipboard()
{
    auto image = mPaintArea->exportAsImage();
    if (image.isNull()) {
        return;
    }
    mClipboard->setImage(image);
}

void MainWindow::setHidden(bool isHidden)
{
    if (isHidden == hidden()) {
        return;
    }

    mHidden = isHidden;
    if (mHidden) {
        setWindowOpacity(0.0);
        showMinimized();
        mPaintArea->setIsEnabled(false);
    } else {
        setWindowOpacity(1.0);
        setWindowState(Qt::WindowActive);
        mPaintArea->setIsEnabled(true);
    }
}

bool MainWindow::hidden() const
{
    return mHidden;
}

/*
 * Default capture trigger, all captures from GUI should happen via this
 * function.
 */
void MainWindow::capture(CaptureModes captureMode)
{
    setHidden(true);
    mConfig->setCaptureMode(captureMode);

    captureScreenshot(captureMode, mConfig->captureCursor(), mConfig->captureDelay());
}

void MainWindow::triggerNewCapture(CaptureModes captureMode)
{
    auto canDiscard = discardUnsavedChanges();
    if (!canDiscard) {
        saveCapture();
        return;
    }
    capture(captureMode);
}

void MainWindow::initGui()
{
    connect(mCaptureModePicker, &CaptureModePicker::captureModeSelected, this, &MainWindow::triggerNewCapture);

    // Create action for save button
    mSaveAction->setText(tr("Save"));
    mSaveAction->setToolTip(tr("Save Screen Capture to file system"));
    mSaveAction->setIcon(IconLoader::loadIcon(QStringLiteral("save")));
    mSaveAction->setShortcut(QKeySequence::Save);
    connect(mSaveAction, &QAction::triggered, this, &MainWindow::saveCapture);

    // Create action for copy to clipboard button
    mCopyToClipboardAction->setText(tr("Copy"));
    mCopyToClipboardAction->setToolTip(tr("Copy Screen Capture to clipboard"));
    mCopyToClipboardAction->setIcon(IconLoader::loadIcon(QStringLiteral("copyToClipboard")));
    mCopyToClipboardAction->setShortcut(QKeySequence::Copy);
    connect(mCopyToClipboardAction, &QAction::triggered, [this]() {
        copyToClipboard();
    });

    // Create Action for imgur.com uploader
    mUploadToImgurAction->setText(tr("Upload"));
    mUploadToImgurAction->setToolTip(tr("Upload capture image to imgur.com"));
    mUploadToImgurAction->setShortcut(Qt::SHIFT + Qt::Key_U);
    connect(mUploadToImgurAction, &QAction::triggered,
            this, &MainWindow::upload);

    // Create print action
    mPrintAction->setText(tr("Print"));
    mPrintAction->setToolTip(tr("Opens printer dialog and provide option to print image"));
    mPrintAction->setShortcut(QKeySequence::Print);
    mPrintAction->setIcon(QIcon::fromTheme(QStringLiteral("document-print")));
    connect(mPrintAction, &QAction::triggered, this, &MainWindow::printClicked);

    // Create print preview action
    mPrintPreviewAction->setText(tr("Print Preview"));
    mPrintPreviewAction->setToolTip(tr("Opens Print Preview dialog where the image "
                                       "orientation can be changed"));
    mPrintPreviewAction->setIcon(QIcon::fromTheme(QStringLiteral("document-print-preview")));
    connect(mPrintPreviewAction, &QAction::triggered,
            this, &MainWindow::printPreviewClicked);

    // Create crop action
    mCropAction->setText(tr("Crop"));
    mCropAction->setToolTip(tr("Crop Screen Capture"));
    mCropAction->setShortcut(Qt::SHIFT + Qt::Key_C);
    connect(mCropAction, &QAction::triggered, this, &MainWindow::openCrop);

    // Create scale action
    mScaleAction->setText(tr("Scale"));
    mScaleAction->setToolTip(tr("Scale Screen Capture"));
    mScaleAction->setShortcut(Qt::SHIFT + Qt::Key_S);
    connect(mScaleAction, &QAction::triggered, this, &MainWindow::openScale);

    // Create action for new capture, this will be only used in the menu bar
    mNewCaptureAction->setText(tr("New"));
    mNewCaptureAction->setShortcut(QKeySequence::New);
    connect(mNewCaptureAction, &QAction::triggered,
            mCaptureModePicker, &CustomToolButton::trigger);

    // Create exit action
    mQuitAction->setText(tr("Quit"));
    mQuitAction->setShortcut(QKeySequence::Quit);
    mQuitAction->setIcon(QIcon::fromTheme(QStringLiteral("application-exit")));
    connect(mQuitAction, &QAction::triggered, this, &MainWindow::close);

    // Create action for opening settings dialog
    mSettingsDialogAction->setText(tr("Settings"));
    mSettingsDialogAction->setIcon(QIcon::fromTheme(QStringLiteral("emblem-system")));
    connect(mSettingsDialogAction, &QAction::triggered, [this]() {
        SettingsDialog settingsDialog(this);
        settingsDialog.exec();
    });

    mAboutKsnipAction->setText(tr("&About"));
    mAboutKsnipAction->setIcon(IconLoader::loadIcon(QStringLiteral("ksnip")));
    connect(mAboutKsnipAction, &QAction::triggered, [this]() {
        AboutDialog aboutDialog(this);
        aboutDialog.exec();
    });

    // Undo and redo actions, the action itself is created in the paintarea
    // class and only a pointer returned here.
    mUndoAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    mUndoAction->setShortcut(QKeySequence::Undo);

    mRedoAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-redo")));
    mRedoAction->setShortcut(QKeySequence::Redo);

    mOpenImageAction->setText(tr("Open"));
    mOpenImageAction->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    mOpenImageAction->setShortcut(Qt::CTRL + Qt::Key_O);
    connect(mOpenImageAction, &QAction::triggered, this, &MainWindow::loadImageFromFile);

    // Create save tool button
    mSaveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mSaveButton->addAction(mSaveAction);
    mSaveButton->setDefaultAction(mSaveAction);

    // Create copy to clipboard tool button
    mCopyToClipboardButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mCopyToClipboardButton->addAction(mCopyToClipboardAction);
    mCopyToClipboardButton->setDefaultAction(mCopyToClipboardAction);

    connect(mToolPicker, &ToolPicker::toolSelected, this, &MainWindow::setPaintModeAndSave);

    // Create painter settings tool button;
    mSettingsButton->setIcon(IconLoader::loadIcon(QStringLiteral("painterSettings")));
    mSettingsButton->setToolTip(tr("Setting Painter tool configuration"));
    connect(mSettingsButton, &SettingsPicker::colorSelected,
            this, &MainWindow::colorChanged);
    connect(mSettingsButton, &SettingsPicker::fillSelected,
            this, &MainWindow::fillChanged);
    connect(mSettingsButton, &SettingsPicker::sizeSelected,
            this, &MainWindow::sizeChanged);

    // Create menu bar
    QMenu* menu;
    menu = menuBar()->addMenu(tr("File"));
    menu->addAction(mNewCaptureAction);
    menu->addAction(mOpenImageAction);
    menu->addAction(mSaveAction);
    menu->addAction(mUploadToImgurAction);
    menu->addSeparator();
    menu->addAction(mPrintAction);
    menu->addAction(mPrintPreviewAction);
    menu->addSeparator();
    menu->addAction(mQuitAction);
    menu = menuBar()->addMenu(tr("&Edit"));
    menu->addAction(mUndoAction);
    menu->addAction(mRedoAction);
    menu->addSeparator();
    menu->addAction(mCopyToClipboardAction);
    menu->addAction(mCropAction);
    menu->addAction(mScaleAction);
    menu = menuBar()->addMenu(tr("&Options"));
    menu->addAction(mSettingsDialogAction);
    menu = menuBar()->addMenu(tr("&Help"));
    menu->addAction(mAboutKsnipAction);

    // Create toolbar
    mToolBar = addToolBar(tr("Tools"));
    mToolBar->setFloatable(false);
    mToolBar->setMovable(false);
    mToolBar->setAllowedAreas(Qt::BottomToolBarArea);
    mToolBar->addWidget(mCaptureModePicker);
    mToolBar->addSeparator();
    mToolBar->addWidget(mSaveButton);
    mToolBar->addWidget(mCopyToClipboardButton);
    mToolBar->addSeparator();
    mToolBar->addWidget(mToolPicker);
    mToolBar->addWidget(mSettingsButton);
    mToolBar->setFixedSize(mToolBar->sizeHint());

    setCentralWidget(mCaptureView);
    resize();
}

void MainWindow::setPaintMode(const PaintMode &mode)
{
    mPaintArea->setPaintMode(mode);
    mSettingsPickerConfigurator->setup(mSettingsButton, mode);

    if (mode == PaintMode::Text) {
        QCoreApplication::instance()->installEventFilter(this);
    } else {
        QCoreApplication::instance()->removeEventFilter(this);
    }
}

void MainWindow::saveCapture()
{
    QString savePath;

    if (mConfig->useInstantSave()) {
        savePath = mConfig->savePath();
    } else {
        QFileDialog saveDialog(this, tr("Save As"), mConfig->savePath(),
                               tr("Images") + QStringLiteral(" (*.png *.gif *.jpg);;") + tr("All Files") + QStringLiteral("(*)"));
        saveDialog.setAcceptMode(QFileDialog::AcceptSave);

        if (saveDialog.exec() == QDialog::Accepted) {
            savePath = saveDialog.selectedFiles().first();
        } else {
            return;
        }
    }

    auto success = mPaintArea->exportAsImage().save(savePath);
    if (!success) {
        qCritical("Unable to save file '%s'", qPrintable(savePath));
        return;
    }

    setSaveAble(false);
}

void MainWindow::upload()
{
    if (!mPaintArea->isValid()) {
        return;
    }

    if (proceedWithUpload()) {
        mCaptureUploader->upload(mPaintArea->exportAsImage());
    }
}

void MainWindow::uploadFinished(QString message)
{
    message = formatUrl(message);

    QDesktopServices::openUrl(message);

    copyToClipboard(message);
}

void MainWindow::printClicked()
{
    mCapturePrinter->print(mConfig->savePath(QStringLiteral("pdf")));
}

void MainWindow::printPreviewClicked()
{
    mCapturePrinter->printPreview(mConfig->savePath(QStringLiteral("pdf")));
}

void MainWindow::setPaintModeAndSave(PaintMode mode)
{
    if (mode != PaintMode::Erase && mode != PaintMode::Select) {
        mConfig->setPaintMode(mode);
    }

    setPaintMode(mode);
}

void MainWindow::instantSave(const QPixmap& pixmap)
{
    QString savePath = mConfig->savePath();

    if (pixmap.save(savePath)) {
        qInfo("Screenshot saved to: %s", qPrintable(savePath));
    } else {
        qCritical("MainWindow::instantSave: Failed to save file at '%s'",
                  qPrintable(savePath));
    }
}

void MainWindow::loadImageFromFile()
{
    auto pixmapFilename = QFileDialog::getOpenFileName(this,
                          tr("Open Image"),
                          mConfig->saveDirectory(),
                          tr("Image Files (*.png *.jpg *.bmp)"));
    QPixmap pixmap(pixmapFilename);
    showCapture(pixmap);
}

void MainWindow::openScale()
{
    auto sceneSize = mPaintArea->areaSize();
    ScaleDialog scaleDialog(sceneSize.width(), sceneSize.height(), this);

    connect(&scaleDialog, &ScaleDialog::finished, [this](int newWidth, int newHeight) {
        mPaintArea->scale(newWidth, newHeight);
    });

    scaleDialog.exec();
}

bool MainWindow::discardUnsavedChanges() const
{
    if (mConfig->promptSaveBeforeExit() && mIsUnsaved) {
        auto reply = MessageBoxHelper::yesNo(tr("Warning - ") + QApplication::applicationName(),
                                             tr("The capture has been modified.\nDo you want to save it?"));

        return !reply;
    } else {
        return true;
    }
}

bool MainWindow::proceedWithUpload() const
{
    if (mConfig->imgurConfirmBeforeUpload()) {
        return MessageBoxHelper::yesNo(tr("Imgur Upload"),
                                       tr("You are about to upload the screenshot to "
                                          "a imgur.com, do you want to proceed?"));
    }
    return true;
}

void MainWindow::copyToClipboard(const QString &message) const
{
    if (mConfig->imgurAlwaysCopyToClipboard()) {
        mClipboard->setText(message);
    }
}

QString &MainWindow::formatUrl(QString &message) const
{
    if (!mConfig->imgurOpenLinkDirectlyToImage()) {
        message = message.remove(QStringLiteral(".png"));
    }
    return message;
}