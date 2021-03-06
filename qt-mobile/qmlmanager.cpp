#include "qmlmanager.h"
#include <QUrl>
#include <QSettings>
#include <QDebug>

#include "qt-models/divelistmodel.h"
#include "divelist.h"
#include "pref.h"
#include "qthelper.h"
#include "qt-gui.h"

static void showMessage(const char *errorString)
{
	if (qqWindowObject && !qqWindowObject->setProperty("messageText", QVariant(errorString)))
		qDebug() << "couldn't set property messageText to" << errorString;
}

QMLManager::QMLManager()
{
	//Initialize cloud credentials.
	setCloudUserName(prefs.cloud_storage_email);
	setCloudPassword(prefs.cloud_storage_password);
	setSaveCloudPassword(prefs.save_password_local);
	if (!same_string(prefs.cloud_storage_email, "") && !same_string(prefs.cloud_storage_password, ""))
		loadDives();
}

QMLManager::~QMLManager()
{
}

void QMLManager::savePreferences()
{
	QSettings s;
	s.beginGroup("CloudStorage");
	s.setValue("email", cloudUserName());
	s.setValue("save_password_local", saveCloudPassword());
	if (saveCloudPassword())
		s.setValue("password", cloudPassword());
	s.sync();
	if (!same_string(prefs.cloud_storage_email, qPrintable(cloudUserName()))) {
		free(prefs.cloud_storage_email);
		prefs.cloud_storage_email = strdup(qPrintable(cloudUserName()));
	}
	if (saveCloudPassword() != prefs.save_password_local) {
		prefs.save_password_local = saveCloudPassword();
	}
	if (saveCloudPassword()) {
		if (!same_string(prefs.cloud_storage_password, qPrintable(cloudPassword()))) {
			free(prefs.cloud_storage_password);
			prefs.cloud_storage_password = strdup(qPrintable(cloudPassword()));
		}
	}
}

void QMLManager::loadDives()
{
	QString url;
	if (getCloudURL(url)) {
		showMessage(get_error_string());
		return;
	}
	clear_dive_file_data();

	QByteArray fileNamePrt  = QFile::encodeName(url);
	int error = parse_file(fileNamePrt.data());
	if (!error) {
		report_error("filename is now %s", fileNamePrt.data());
		showMessage(get_error_string());
		set_filename(fileNamePrt.data(), true);
	} else {
		showMessage(get_error_string());
	}
	process_dives(false, false);

	int i;
	struct dive *d;

	for_each_dive(i, d) {
		DiveListModel::instance()->addDive(d);
	}
}

void QMLManager::commitChanges(QString diveId, QString suit, QString buddy, QString diveMaster, QString notes)
{
	struct dive *d = get_dive_by_uniq_id(diveId.toInt());
	bool diveChanged = false;

	if (d->suit != suit.toUtf8().data()) {
		diveChanged = true;
		free(d->suit);
		d->suit = strdup(suit.toUtf8().data());
	}
	if (d->buddy != buddy.toUtf8().data()) {
		diveChanged = true;
		free(d->buddy);
		d->buddy = strdup(buddy.toUtf8().data());
	}
	if (d->divemaster != diveMaster.toUtf8().data()) {
		diveChanged = true;
		free(d->divemaster);
		d->divemaster = strdup(diveMaster.toUtf8().data());
	}
	if (d->notes != notes.toUtf8().data()) {
		diveChanged = true;
		free(d->notes);
		d->notes = strdup(notes.toUtf8().data());
	}
}

void QMLManager::saveChanges()
{
	showMessage("Saving dives.");
	QString fileName;
	if (getCloudURL(fileName)) {
		showMessage(get_error_string());
		return;
	}

	if (save_dives(fileName.toUtf8().data())) {
		showMessage(get_error_string());
		return;
	}

	showMessage("Dives saved.");
	set_filename(fileName.toUtf8().data(), true);
	mark_divelist_changed(false);
}
bool QMLManager::saveCloudPassword() const
{
	return m_saveCloudPassword;
}

void QMLManager::setSaveCloudPassword(bool saveCloudPassword)
{
	m_saveCloudPassword = saveCloudPassword;
}


QString QMLManager::cloudPassword() const
{
	return m_cloudPassword;
}

void QMLManager::setCloudPassword(const QString &cloudPassword)
{
	m_cloudPassword = cloudPassword;
	emit cloudPasswordChanged();
}

QString QMLManager::cloudUserName() const
{
	return m_cloudUserName;
}

void QMLManager::setCloudUserName(const QString &cloudUserName)
{
	m_cloudUserName = cloudUserName;
	emit cloudUserNameChanged();
}
