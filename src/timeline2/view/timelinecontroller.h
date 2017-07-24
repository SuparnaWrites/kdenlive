/***************************************************************************
 *   Copyright (C) 2017 by Jean-Baptiste Mardelle                                  *
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

#ifndef TIMELINECONTROLLER_H
#define TIMELINECONTROLLER_H

#include "timeline2/model/timelineitemmodel.hpp"
#include "timelinewidget.h"

// see https://bugreports.qt.io/browse/QTBUG-57714, don't expose a QWidget as a context property
class TimelineController : public QObject
{
    Q_OBJECT
    /* @brief holds a list of currently selected clips (list of clipId's)
     */
    Q_PROPERTY(QList<int> selection READ selection WRITE setSelection NOTIFY selectionChanged)
    /* @brief holds the timeline zoom factor
     */
    Q_PROPERTY(double scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY scaleFactorChanged)
    /* @brief holds the current project duration
     */
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool audioThumbFormat READ audioThumbFormat NOTIFY audioThumbFormatChanged)
    /* @brief holds the current timeline position
     */
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(int zoneIn READ zoneIn WRITE setZoneIn NOTIFY zoneChanged)
    Q_PROPERTY(int zoneOut READ zoneOut WRITE setZoneOut NOTIFY zoneChanged)
    Q_PROPERTY(int seekPosition READ seekPosition WRITE setSeekPosition NOTIFY seekPositionChanged)
    Q_PROPERTY(bool ripple READ ripple NOTIFY rippleChanged)
    Q_PROPERTY(bool scrub READ scrub NOTIFY scrubChanged)
    Q_PROPERTY(bool showThumbnails READ showThumbnails NOTIFY showThumbnailsChanged)
    Q_PROPERTY(bool showMarkers READ showMarkers NOTIFY showMarkersChanged)
    Q_PROPERTY(bool showAudioThumbnails READ showAudioThumbnails NOTIFY showAudioThumbnailsChanged)

public:
    TimelineController(KActionCollection *actionCollection, QObject *parent);

    /* @brief Sets the model that this widgets displays */
    void setModel(std::shared_ptr<TimelineItemModel> model);
    std::shared_ptr<TimelineItemModel> getModel() const;
    void setRoot(QQuickItem *root);

    Q_INVOKABLE bool isMultitrackSelected() const { return m_selection.isMultitrackSelected; }
    Q_INVOKABLE int selectedTrack() const { return m_selection.selectedTrack; }
    /* @brief Add a clip id to current selection
     */
    Q_INVOKABLE void addSelection(int newSelection);
    /* @brief returns current timeline's zoom factor
     */
    Q_INVOKABLE double scaleFactor() const;
    /* @brief set current timeline's zoom factor
     */
    Q_INVOKABLE void setScaleFactor(double scale);
    /* @brief Returns the project's duration (tractor)
     */
    Q_INVOKABLE int duration() const;
    /* @brief Returns the current cursor position (frame currently displayed by MLT)
     */
    Q_INVOKABLE int position() const { return m_position; }
    /* @brief Returns the seek request position (-1 = no seek pending)
     */
    Q_INVOKABLE int seekPosition() const { return m_seekPosition; }
    /* @brief Request a seek operation
       @param position is the desired new timeline position
     */
    Q_INVOKABLE int zoneIn() const { return m_zone.x(); }
    Q_INVOKABLE int zoneOut() const { return m_zone.y(); }
    Q_INVOKABLE void setZoneIn(int inPoint);
    Q_INVOKABLE void setZoneOut(int outPoint);
    void setZone(const QPoint &zone);
    /* @brief Request a seek operation
       @param position is the desired new timeline position
     */
    Q_INVOKABLE void setPosition(int position);
    Q_INVOKABLE bool snap();
    Q_INVOKABLE bool ripple();
    Q_INVOKABLE bool scrub();
    Q_INVOKABLE QString timecode(int frames);
    /* @brief Request inserting a new clip in timeline (dragged from bin or monitor)
       @param tid is the destination track
       @param position is the timeline position
       @param xml is the data describing the dropped clip
       @param logUndo if set to false, no undo object is stored
       @return the id of the inserted clip
     */
    Q_INVOKABLE int insertClip(int tid, int position, const QString &xml, bool logUndo);

    /* @brief Request inserting a new composition in timeline (dragged from compositions list)
       @param tid is the destination track
       @param position is the timeline position
       @param transitionId is the data describing the dropped composition
       @param logUndo if set to false, no undo object is stored
       @return the id of the inserted composition
    */
    Q_INVOKABLE int insertComposition(int tid, int position, const QString &transitionId, bool logUndo);

    /* @brief Request deletion of the currently selected clips
     */
    Q_INVOKABLE void deleteSelectedClips();

    Q_INVOKABLE void triggerAction(const QString &name);

    /* @brief Do we want to display video thumbnails
     */
    bool showThumbnails() const;
    bool showAudioThumbnails() const;
    bool showMarkers() const;
    bool audioThumbFormat() const;
    /* @brief Do we want to display audio thumbnails
     */
    Q_INVOKABLE bool showWaveforms() const;
    /* @brief Insert a timeline track
     */
    Q_INVOKABLE void addTrack(int tid);
    /* @brief Remove a timeline track
     */
    Q_INVOKABLE void deleteTrack(int tid);
    /* @brief Group selected items in timeline
     */
    Q_INVOKABLE void groupSelection();
    /* @brief Ungroup selected items in timeline
     */
    Q_INVOKABLE void unGroupSelection(int cid);
    /* @brief Ask for edit marker dialog
     */
    Q_INVOKABLE void editMarker(const QString &cid, int frame);
    /* @brief Ask for edit timeline guide dialog
     */
    Q_INVOKABLE void editGuide(int frame = -1);
    /* @brief Add a timeline guide
     */
    Q_INVOKABLE void switchGuide(int frame = -1, bool deleteOnly = false);
    /* @brief Request monitor refresh
     */
    Q_INVOKABLE void requestRefresh();

    /* @brief Show the asset of the given item in the AssetPanel
       If the id corresponds to a clip, we show the corresponding effect stack
       If the id corresponds to a composition, we show its properties
    */
    Q_INVOKABLE void showAsset(int id);

    /* @brief Seek to next snap point
     */
    void gotoNextSnap();
    /* @brief Seek to previous snap point
     */
    void gotoPreviousSnap();
    /* @brief Set current item's start point to cursor position
     */
    void setInPoint();
    /* @brief Set current item's end point to cursor position
     */
    void setOutPoint();
    /* @brief Return the project's tractor
     */
    Mlt::Tractor *tractor();
    /* @brief Sets the list of currently selected clips
       @param selection is the list of id's
       @param trackIndex is current clip's track
       @param isMultitrack is true if we want to select the whole tractor (currently unused)
     */
    void setSelection(const QList<int> &selection = QList<int>(), int trackIndex = -1, bool isMultitrack = false);
    /* @brief Get the list of currenly selected clip id's
     */
    QList<int> selection() const;
    /* @brief Add an asset (effect, composition)
     */
    void addAsset(const QVariantMap data);
    void checkDuration();
    /* @brief Cuts the clip on current track at timeline position
     */
    Q_INVOKABLE void cutClipUnderCursor(int position = -1, int track = -1);
    /* @brief Request a spacer operation
     */
    Q_INVOKABLE int requestSpacerStartOperation(int trackId, int position);
    /* @brief Request a spacer operation
     */
    Q_INVOKABLE bool requestSpacerEndOperation(int clipId, int startPosition, int endPosition);
    /* @brief Request a Fade in effect for clip
     */
    Q_INVOKABLE void adjustFade(int cid, const QString &effectId, int duration);

    Q_INVOKABLE const QString getTrackNameFromMltIndex(int trackPos);
    /* @brief Seeks to selected clip start / end
     */
    void seekCurrentClip(bool seekToEnd = false);
    /* @brief Returns the number of tracks (audioTrakcs, videoTracks)
     */
    QPoint getTracksCount() const;
    /* @brief Request monitor refresh if item (clip or composition) is under timeline cursor
     */
    void refreshItem(int id);
    /* @brief Seek timeline to mouse position
     */
    void seekToMouse();

    /* @brief User enabled / disabled snapping, update timeline behavior
     */
    void snapChanged(bool snap);

    /* @brief Returns a list of all luma files used in the project
     */
    QStringList extractCompositionLumas() const;
    /* @brief Get the frame where mouse is positionned
     */
    int getMousePos();
    /* @brief Returns a map of track ids/track names
     */
    QMap<int, QString> getTrackNames(bool videoOnly);
    /* @brief Returns the transition a track index for a composition
     */
    int getCompositionATrack(int cid) const;
    void setCompositionATrack(int cid, int aTrack);
    const QString getClipBinId(int clipId) const;

public slots:
    void selectMultitrack();
    Q_INVOKABLE void setSeekPosition(int position);
    void onSeeked(int position);
    void addEffectToCurrentClip(const QStringList &effectData);

private:
    QQuickItem *m_root;
    KActionCollection *m_actionCollection;
    std::shared_ptr<TimelineItemModel> m_model;
    struct Selection
    {
        QList<int> selectedClips;
        int selectedTrack;
        bool isMultitrackSelected;
    };
    int m_position;
    int m_seekPosition;
    QPoint m_zone;
    double m_scale;
    static int m_duration;
    Selection m_selection;
    Selection m_savedSelection;
    void emitSelectedFromSelection();

signals:
    void selected(Mlt::Producer *producer);
    void selectionChanged();
    void frameFormatChanged();
    void trackHeightChanged();
    void scaleFactorChanged();
    void audioThumbFormatChanged();
    void durationChanged();
    void positionChanged();
    void seekPositionChanged();
    void showThumbnailsChanged();
    void showAudioThumbnailsChanged();
    void showMarkersChanged();
    void rippleChanged();
    void scrubChanged();
    void seeked(int position);
    void zoneChanged();
    void zoneMoved(const QPoint &zone);
    /* @brief Requests that a given parameter model is displayed in the asset panel */
    void showTransitionModel(int tid, std::shared_ptr<AssetParameterModel>);
    void showClipEffectStack(const QString &clipName, std::shared_ptr<EffectStackModel>, QPair<int, int> range);
};

#endif
