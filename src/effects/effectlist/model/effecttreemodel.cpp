/*
    SPDX-FileCopyrightText: 2017 Nicolas Carion
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "effecttreemodel.hpp"
#include "abstractmodel/treeitem.hpp"
#include "effects/effectsrepository.hpp"
#include "kdenlivesettings.h"
#include <KLocalizedString>
#include <QDomDocument>
#include <QFile>
#include <array>
#include <vector>

#include <KActionCategory>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>

EffectTreeModel::EffectTreeModel(QObject *parent)
    : AssetTreeModel(parent)
    , m_customCategory(nullptr)
    , m_templateCategory(nullptr)
{
}

std::shared_ptr<EffectTreeModel> EffectTreeModel::construct(const QString &categoryFile, QObject *parent)
{
    std::shared_ptr<EffectTreeModel> self(new EffectTreeModel(parent));
    QList<QVariant> rootData {"Name", "ID", "Type", "isFav"};
    self->rootItem = TreeItem::construct(rootData, self, true);

    QHash<QString, std::shared_ptr<TreeItem>> effectCategory; // category in which each effect should land.

    std::shared_ptr<TreeItem> miscCategory = nullptr;
    std::shared_ptr<TreeItem> audioCategory = nullptr;
    // We parse category file
    if (!categoryFile.isEmpty()) {
        QDomDocument doc;
        QFile file(categoryFile);
        doc.setContent(&file, false);
        file.close();
        QDomNodeList groups = doc.documentElement().elementsByTagName(QStringLiteral("group"));
        auto groupLegacy = self->rootItem->appendChild(QList<QVariant>{i18n("Legacy"), QStringLiteral("root")});

        for (int i = 0; i < groups.count(); i++) {
            QString groupName = i18n(groups.at(i).firstChild().firstChild().nodeValue().toUtf8().constData());
            if (!KdenliveSettings::gpu_accel() && groupName == i18n("GPU effects")) {
                continue;
            }
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            QStringList list = groups.at(i).toElement().attribute(QStringLiteral("list")).split(QLatin1Char(','), QString::SkipEmptyParts);
#else
            QStringList list = groups.at(i).toElement().attribute(QStringLiteral("list")).split(QLatin1Char(','), Qt::SkipEmptyParts);
#endif
            auto groupItem = self->rootItem->appendChild(QList<QVariant>{groupName, QStringLiteral("root")});
            for (const QString &effect : qAsConst(list)) {
                effectCategory[effect] = groupItem;
            }
        }
        // We also create "Misc", "Audio" and "Custom" categories
        miscCategory = self->rootItem->appendChild(QList<QVariant>{i18n("Misc"), QStringLiteral("root")});
        audioCategory = self->rootItem->appendChild(QList<QVariant>{i18n("Audio"), QStringLiteral("root")});
        self->m_customCategory = self->rootItem->appendChild(QList<QVariant>{i18n("Custom"), QStringLiteral("root")});
        self->m_templateCategory = self->rootItem->appendChild(QList<QVariant>{i18n("Templates"), QStringLiteral("root")});
    } else {
        // Flat view
        miscCategory = self->rootItem;
        audioCategory = self->rootItem;
        self->m_customCategory = self->rootItem;
        self->m_templateCategory = self->rootItem;
    }

    // We parse effects
    auto allEffects = EffectsRepository::get()->getNames();
    QString favCategory = QStringLiteral("kdenlive:favorites");
    for (const auto &effect : qAsConst(allEffects)) {
        auto targetCategory = miscCategory;
        AssetListType::AssetType type = EffectsRepository::get()->getType(effect.first);
        if (effectCategory.contains(effect.first)) {
            targetCategory = effectCategory[effect.first];
        } else if (type == AssetListType::AssetType::Audio) {
            targetCategory = audioCategory;
        }

        if (type == AssetListType::AssetType::Custom || type == AssetListType::AssetType::CustomAudio) {
            targetCategory = self->m_customCategory;
        }
        if (type == AssetListType::AssetType::Template || type == AssetListType::AssetType::TemplateAudio) {
            targetCategory = self->m_templateCategory;
        }

        // we create the data list corresponding to this profile
        bool isFav = KdenliveSettings::favorite_effects().contains(effect.first);
        bool isPreferred = EffectsRepository::get()->isPreferred(effect.first);
        QList<QVariant> data;
        if (targetCategory->dataColumn(0).toString() == i18n("Deprecated")) {
            QString updatedName = effect.second + i18n(" - deprecated");
            data = {updatedName, effect.first, QVariant::fromValue(type), isFav, targetCategory->row(), isPreferred};
        } else {
            //qDebug() << effect.second << effect.first << "in " << targetCategory->dataColumn(0).toString();
            data = {effect.second, effect.first, QVariant::fromValue(type), isFav, targetCategory->row(), isPreferred};
        }
        if (KdenliveSettings::favorite_effects().contains(effect.first) && effectCategory.contains(favCategory)) {
            targetCategory = effectCategory[favCategory];
        }
        targetCategory->appendChild(data);
    }
    return self;
}

void EffectTreeModel::reloadEffectFromIndex(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    std::shared_ptr<TreeItem> item = getItemById(int(index.internalId()));
    const QString path = EffectsRepository::get()->getCustomPath(item->dataColumn(IdCol).toString());
    reloadEffect(path);
}

void EffectTreeModel::reloadEffect(const QString &path)
{
    QPair<QString, QString> asset = EffectsRepository::get()->reloadCustom(path);
    if (asset.first.isEmpty() || m_customCategory == nullptr) {
        return;
    }
    // Check if item already existed, and remove
    for (int i = 0; i < m_customCategory->childCount(); i++) {
        std::shared_ptr<TreeItem> item = m_customCategory->child(i);
        if (item->dataColumn(IdCol).toString() == asset.first) {
            m_customCategory->removeChild(item);
            break;
        }
    }
    bool isFav = KdenliveSettings::favorite_effects().contains(asset.first);
    QString effectName = EffectsRepository::get()->getName(asset.first);
    QList<QVariant> data {effectName, asset.first, QVariant::fromValue(AssetListType::AssetType::Custom), isFav};
    m_customCategory->appendChild(data);
}

void EffectTreeModel::deleteEffect(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    std::shared_ptr<TreeItem> item = getItemById(int(index.internalId()));
    const QString id = item->dataColumn(IdCol).toString();
    m_customCategory->removeChild(item);
    EffectsRepository::get()->deleteEffect(id);
}

void EffectTreeModel::reloadAssetMenu(QMenu *effectsMenu, KActionCategory *effectActions)
{
    for (int i = 0; i < rowCount(); i++) {
        std::shared_ptr<TreeItem> item = rootItem->child(i);
        if (item->childCount() > 0) {
            QMenu *catMenu = new QMenu(item->dataColumn(AssetTreeModel::NameCol).toString(), effectsMenu);
            effectsMenu->addMenu(catMenu);
            for (int j = 0; j < item->childCount(); j++) {
                std::shared_ptr<TreeItem> child = item->child(j);
                QAction *a = new QAction(i18n(child->dataColumn(AssetTreeModel::NameCol).toString().toUtf8().data()), catMenu);
                const QString id = child->dataColumn(AssetTreeModel::IdCol).toString();
                a->setData(id);
                catMenu->addAction(a);
                effectActions->addAction("transition_" + id, a);
            }
        }
    }
}

void EffectTreeModel::setFavorite(const QModelIndex &index, bool favorite, bool isEffect)
{
    if (!index.isValid()) {
        return;
    }
    std::shared_ptr<TreeItem> item = getItemById(int(index.internalId()));
    if (isEffect && item->depth() == 1) {
        return;
    }
    item->setData(AssetTreeModel::FavCol, favorite);
    auto id = item->dataColumn(AssetTreeModel::IdCol).toString();
    if (!EffectsRepository::get()->exists(id)) {
        qDebug()<<"Trying to reparent unknown asset: "<<id;
        return;
    }
    QStringList favs = KdenliveSettings::favorite_effects();
    if (!favorite) {
        favs.removeAll(id);
    } else {
        favs << id;
    }
    KdenliveSettings::setFavorite_effects(favs);
}

void EffectTreeModel::editCustomAsset(const QString &newName,const QString &newDescription, const QModelIndex &index)
{

    std::shared_ptr<TreeItem> item = getItemById(int(index.internalId()));
    QString currentName = item->dataColumn(AssetTreeModel::NameCol).toString();

    QDomDocument doc;

    QDomElement effect = EffectsRepository::get()->getXml(currentName);
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/effects/"));
    QString oldpath = dir.absoluteFilePath(currentName + QStringLiteral(".xml"));

    doc.appendChild(doc.importNode(effect, true));

    if(!newDescription.trimmed().isEmpty()){
        QDomElement root = doc.documentElement();
        QDomElement nodelist = root.firstChildElement("description");
        QDomElement newNodeTag = doc.createElement(QString("description"));
        QDomText text = doc.createTextNode(newDescription);
        newNodeTag.appendChild(text);
        if (!nodelist.isNull()) {
            root.replaceChild(newNodeTag, nodelist);
        } else {
            root.appendChild(newNodeTag);
        }
    }

    if(!newName.trimmed().isEmpty() && newName != currentName)
    {
        if (!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }

        if (dir.exists(newName + QStringLiteral(".xml"))){
            QMessageBox message;
            message.critical(nullptr, i18n("Error"), i18n("Effect name %1 already exists.\n Try another name?", newName));
            message.setFixedSize(400, 200);
            return;
        }
        QFile file(dir.absoluteFilePath(newName + QStringLiteral(".xml")));

        QDomElement root = doc.documentElement();
        QDomElement nodelist = root.firstChildElement("name");
        QDomElement newNodeTag = doc.createElement(QString("name"));
        QDomText text = doc.createTextNode(newName);
        newNodeTag.appendChild(text);
        root.replaceChild(newNodeTag, nodelist);

        QDomElement e = doc.documentElement();
        e.setAttribute("id", newName);

        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << doc.toString();
        }
        file.close();

        deleteEffect(index);
        reloadEffect(dir.absoluteFilePath(newName + QStringLiteral(".xml")));

    }
    else
    {
        QFile file(dir.absoluteFilePath(currentName + QStringLiteral(".xml")));
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << doc.toString();
        }
        file.close();
        reloadEffect(oldpath);
    }

}
