#include "languageModel.h"

LanguageModel::LanguageModel(QObject *parent) : QAbstractListModel(parent)
{
    for (const auto language : { LanguageSettings::AvailableLanguageEnum::English,
                                 LanguageSettings::AvailableLanguageEnum::China_cn }) {
        m_availableLanguages.push_back(LanguageModelData { getLocalLanguageName(language), language });
    }
}

int LanguageModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(m_availableLanguages.size());
}

QVariant LanguageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_availableLanguages.size())) {
        return {};
    }

    switch (role) {
    case NameRole: return m_availableLanguages[index.row()].name;
    case IndexRole: return static_cast<int>(m_availableLanguages[index.row()].index);
    }
    return {};
}

QHash<int, QByteArray> LanguageModel::roleNames() const
{
    return { { NameRole, "languageName" }, { IndexRole, "languageIndex" } };
}

QString LanguageModel::getLocalLanguageName(const LanguageSettings::AvailableLanguageEnum language)
{
    return language == LanguageSettings::AvailableLanguageEnum::China_cn
        ? "\347\256\200\344\275\223\344\270\255\346\226\207"
        : "English";
}
