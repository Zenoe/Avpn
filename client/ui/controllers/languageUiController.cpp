#include "languageUiController.h"

LanguageUiController::LanguageUiController(SettingsController* settingsController,
                                           LanguageModel* languageModel,
                                           QObject *parent)
    : QObject(parent),
      m_settingsController(settingsController),
      m_languageModel(languageModel)
{
}

void LanguageUiController::onAppLanguageChanged(const QLocale &locale)
{
    emit updateTranslations(locale);
}

void LanguageUiController::changeLanguage(const LanguageSettings::AvailableLanguageEnum language)
{
    m_settingsController->setAppLanguage(languageEnumToLocale(language));
}

int LanguageUiController::getCurrentLanguageIndex() const
{
    return m_settingsController->getAppLanguage().language() == QLocale::Chinese
        ? static_cast<int>(LanguageSettings::AvailableLanguageEnum::China_cn)
        : static_cast<int>(LanguageSettings::AvailableLanguageEnum::English);
}

int LanguageUiController::getLineHeightAppend() const
{
    return 0;
}

QString LanguageUiController::getCurrentLanguageName() const
{
    return getLocalLanguageName(static_cast<LanguageSettings::AvailableLanguageEnum>(getCurrentLanguageIndex()));
}

LanguageSettings::AvailableLanguageEnum LanguageUiController::getSystemLanguageEnum() const
{
    return QLocale::system().language() == QLocale::Chinese
        ? LanguageSettings::AvailableLanguageEnum::China_cn
        : LanguageSettings::AvailableLanguageEnum::English;
}

QString LanguageUiController::getCurrentSiteUrl(const QString &path) const
{
    return QString("https://amnezia.org") + (path.isEmpty() ? "" : QString("/%1").arg(path));
}

QString LanguageUiController::getCurrentDocsUrl(const QString &path) const
{
    return QString("https://docs.amnezia.org") + (path.isEmpty() ? "" : QString("/%1").arg(path));
}

QString LanguageUiController::getLocalLanguageName(const LanguageSettings::AvailableLanguageEnum language) const
{
    switch (language) {
    case LanguageSettings::AvailableLanguageEnum::English: return "English";
    case LanguageSettings::AvailableLanguageEnum::China_cn: return "\347\256\200\344\275\223\344\270\255\346\226\207";
    }
    return {};
}

QLocale LanguageUiController::languageEnumToLocale(const LanguageSettings::AvailableLanguageEnum language) const
{
    return language == LanguageSettings::AvailableLanguageEnum::China_cn ? QLocale::Chinese : QLocale::English;
}
