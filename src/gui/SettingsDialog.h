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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QFontComboBox>
#include <QListWidget>
#include <QGroupBox>
#include <QStackedLayout>
#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>

#include "src/widgets/NumericComboBox.h"
#include "src/widgets/ColorButton.h"
#include "src/backend/KsnipConfig.h"
#include "src/common/formatter/FilenameFormatter.h"
#include "src/backend/ImgurUploader.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog() override;

private:
    QCheckBox       *mAlwaysCopyToClipboardCheckbox;
    QCheckBox       *mPromptToSaveBeforeExitCheckbox;
    QCheckBox       *mSaveKsnipPositionCheckbox;
    QCheckBox       *mSaveKsnipToolSelectionCheckbox;
    QCheckBox       *mCaptureOnStartupCheckbox;
    QCheckBox       *mCaptureCursorCheckbox;
    QCheckBox       *mUseInstantSaveCheckBox;
    QCheckBox       *mImgurForceAnonymousCheckbox;
    QCheckBox       *mImgurDirectLinkToImageCheckbox;
    QCheckBox       *mImgurAlwaysCopyToClipboardCheckBox;
    QCheckBox       *mImgurConfirmBeforeUploadCheckbox;
    QCheckBox       *mSmoothPathCheckbox;
    QCheckBox       *mItemShadowCheckbox;
    QCheckBox       *mCursorRulerCheckbox;
    QCheckBox       *mCursorInfoCheckbox;
    QCheckBox       *mDynamicCursorSize;
    QLineEdit       *mSaveLocationLineEdit;
    QLineEdit       *mImgurClientIdLineEdit;
    QLineEdit       *mImgurClientSecretLineEdit;
    QLineEdit       *mImgurPinLineEdit;
    QLabel          *mCaptureDelayLabel;
    QLabel          *mSaveLocationLabel;
    QLabel          *mImgurUsernameLabel;
    QLabel          *mTextFontLabel;
    QLabel          *mNumberFontLabel;
    QLabel          *mSmoothFactorLabel;
    QLabel          *mSnippingCursorSizeLabel;
    QLabel          *mSnippingCursorColorLabel;
    NumericComboBox *mCaptureDelayCombobox;
    NumericComboBox *mSmoothFactorCombobox;
    NumericComboBox *mSnippingCursorSizeCombobox;
    QFontComboBox   *mTextFontCombobox;
    QFontComboBox   *mNumberFontCombobox;
    QPushButton     *mBrowseButton;
    QPushButton     *mImgurGetPinButton;
    QPushButton     *mImgurGetTokenButton;
    QPushButton     *mOkButton;
    QPushButton     *mCancelButton;
    QPushButton     *mTextBoldButton;
    QPushButton     *mTextItalicButton;
    QPushButton     *mTextUnderlineButton;
    ColorButton     *mSnippingCursorColorButton;
    ImgurUploader   *mImgurUploader;
    QListWidget     *mListWidget;
    QStackedLayout  *mStackedLayout;
    KsnipConfig     *mConfig;

    void loadSettings();
    void saveSettings();
    void initGui();

private slots:
    void requestImgurPin();
    void getImgurToken();
    void smootPathCheckboxClicked(bool checked);
    void imgurClientEntered(const QString &text);
    void imgurTokenUpdated(const QString &accessToken, const QString &refreshTocken, const QString &username);
    void imgurTokenError(const QString &message);
    void chooseSaveDirectory();
};

#endif // SETTINGSDIALOG_H
