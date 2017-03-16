/***************************************************************************
 *   Copyright (C) 2017 by Nicolas Carion                                  *
 *   This file is part of Kdenlive. See www.kdenlive.org.                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3 or any later version accepted by the       *
 *   membership of KDE e.V. (or its successor approved  by the membership  *
 *   of KDE e.V.), which shall act as a proxy defined in Section 14 of     *
 *   version 3 of the license.                                             *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "effectfilter.hpp"
#include "effecttreemodel.hpp"
#include "abstractmodel/treeitem.hpp"

EffectFilter::EffectFilter(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_name_enabled = m_type_enabled = false;
}

void EffectFilter::setFilterName(bool enabled, const QString& pattern)
{
    m_name_enabled = enabled;
    m_name_value = pattern;
    invalidateFilter();
}


bool EffectFilter::filterName(TreeItem* item) const
{
    if (!m_name_enabled) {
        return true;
    }
    QString itemText = item->data(EffectTreeModel::nameCol).toString();
    itemText = itemText.normalized(QString::NormalizationForm_D).remove(QRegExp(QStringLiteral("[^a-zA-Z0-9\\s]")));
    QString patt = m_name_value.normalized(QString::NormalizationForm_D).remove(QRegExp(QStringLiteral("[^a-zA-Z0-9\\s]")));

    return itemText.contains(patt, Qt::CaseInsensitive);
}

void EffectFilter::setFilterType(bool enabled, EffectType type)
{
    m_type_enabled = enabled;
    m_type_value = type;
    invalidateFilter();
}


bool EffectFilter::filterType(TreeItem* item) const
{
    if (!m_type_enabled) {
        return true;
    }
    EffectType itemType = item->data(EffectTreeModel::typeCol).value<EffectType>();
    return itemType == m_type_value;
}

bool EffectFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (sourceParent == QModelIndex()) {
        //In that case, we have a category. We hide it if it does not have children.
        QModelIndex category = sourceModel()->index(sourceRow, 0, sourceParent);
        bool accepted = false;
        for (int i = 0; i < sourceModel()->rowCount(category) && !accepted; ++i) {
            accepted = filterAcceptsRow(i, category);
        }
        return accepted;
    }
    QModelIndex row = sourceModel()->index(sourceRow, 0, sourceParent);
    TreeItem *item = static_cast<TreeItem*>(row.internalPointer());

    return filterName(item) && filterType(item);
}

bool EffectFilter::isVisible(const QModelIndex &sourceIndex)
{
    auto parent = sourceModel()->parent(sourceIndex);
    return filterAcceptsRow(sourceIndex.row(), parent);
}
