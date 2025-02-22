/*
    SPDX-FileCopyrightText: 2019 Jean-Baptiste Mardelle
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "bin/model/markerlistmodel.hpp"
#include "clipsnapmodel.hpp"
#include <climits>
#include <cstdlib>
#include <memory>

ClipSnapModel::ClipSnapModel() = default;

void ClipSnapModel::addPoint(int position)
{
    m_snapPoints.insert(position);
    if (position < m_inPoint * m_speed || position >= m_outPoint * m_speed) {
        return;
    }
    if (auto ptr = m_registeredSnap.lock()) {
        ptr->addPoint(m_speed < 0 ? int(ceil(m_outPoint + m_position + position / m_speed - m_inPoint)) : int(ceil(m_position + position / m_speed - m_inPoint)));
    }
}

void ClipSnapModel::removePoint(int position)
{
    m_snapPoints.erase(position);
    if (position < m_inPoint * m_speed || position >= m_outPoint * m_speed) {
        return;
    }
    if (auto ptr = m_registeredSnap.lock()) {
        ptr->removePoint(m_speed < 0 ? int(ceil(m_outPoint + m_position + position / m_speed - m_inPoint)) : int(ceil(m_position + position / m_speed - m_inPoint)));
    }
}

void ClipSnapModel::updateSnapModelPos(int newPos)
{
    if (newPos == m_position) {
        return;
    }
    removeAllSnaps();
    m_position = newPos;
    addAllSnaps();
}

void ClipSnapModel::updateSnapModelInOut(std::vector<int> borderSnaps)
{
    removeAllSnaps();
    m_inPoint = borderSnaps.at(0);
    m_outPoint = borderSnaps.at(1);
    m_mixPoint = borderSnaps.at(2);
    addAllSnaps();
}

void ClipSnapModel::updateSnapMixPosition(int mixPos)
{
    removeAllSnaps();
    m_mixPoint = mixPos;
    addAllSnaps();
}

void ClipSnapModel::addAllSnaps()
{
    if (auto ptr = m_registeredSnap.lock()) {
        for (const auto &snap : m_snapPoints) {
            if (snap >= m_inPoint * m_speed && snap < m_outPoint * m_speed) {
                ptr->addPoint(m_speed < 0 ? int(ceil(m_outPoint + m_position + snap / m_speed - m_inPoint)) : int(ceil(m_position + snap / m_speed - m_inPoint)));
            }
        }
        if (m_mixPoint > 0) {
            ptr->addPoint(int(ceil(m_position + m_mixPoint)));
        }
    }
}

void ClipSnapModel::removeAllSnaps()
{
    if (auto ptr = m_registeredSnap.lock()) {
        for (const auto &snap : m_snapPoints) {
            if (snap >= m_inPoint * m_speed && snap < m_outPoint * m_speed) {
                ptr->removePoint(m_speed < 0 ? int(ceil(m_outPoint + m_position + snap / m_speed - m_inPoint)) : int(ceil(m_position + snap / m_speed - m_inPoint)));
            }
        }
        if (m_mixPoint > 0) {
            ptr->removePoint(int(ceil(m_position + m_mixPoint)));
        }
    }
}

void ClipSnapModel::allSnaps(std::vector<int> &snaps, int offset) const
{
    snaps.push_back(m_position - offset);
    if (auto ptr = m_registeredSnap.lock()) {
        for (const auto &snap : m_snapPoints) {
            if (snap >= m_inPoint * m_speed && snap < m_outPoint * m_speed) {
                snaps.push_back(m_speed < 0 ? int(ceil(m_outPoint + m_position + snap / m_speed - m_inPoint - offset)) : int(ceil(m_position + snap / m_speed - m_inPoint - offset)));
            }
        }
    }
    if (m_mixPoint > 0) {
        snaps.push_back(m_position + m_mixPoint - offset);
    }
    snaps.push_back(m_position + m_outPoint - m_inPoint + 1 - offset);
}

void ClipSnapModel::registerSnapModel(const std::weak_ptr<SnapModel> &snapModel, int position, int in, int out, double speed)
{
    // make sure ptr is valid
    m_inPoint = in;
    m_outPoint = out;
    m_speed = speed;
    m_position = qMax(0, position);
    m_registeredSnap = snapModel;
    addAllSnaps();
}

void ClipSnapModel::deregisterSnapModel()
{
    // make sure ptr is valid
    removeAllSnaps();
    m_registeredSnap.reset();
}

void ClipSnapModel::setReferenceModel(const std::weak_ptr<MarkerListModel> &markerModel, double speed)
{
    m_parentModel = markerModel;
    m_speed = speed;
    if (auto ptr = m_parentModel.lock()) {
        ptr->registerSnapModel(std::static_pointer_cast<SnapInterface>(shared_from_this()));
    }
}
