/***************************************************************************
                          kmmtracksoundpanel.cpp  -  description
                             -------------------
    begin                : Tue Apr 9 2002
    copyright            : (C) 2002 by Jason Wood
    email                : jasonwood@blueyonder.co.uk
 ***************************************************************************/
/*decorators added by MArco Gittler (g.marco@freenet.de)*/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qsizepolicy.h>
#include <qtoolbutton.h>
#include <kiconloader.h>

#include "kmmtracksoundpanel.h"
#include "ktrackplacer.h"
#include "klocale.h"
#include "kdenlivesettings.h"

#include "trackviewbackgrounddecorator.h"
#include "trackviewaudiobackgrounddecorator.h"
#include "trackviewnamedecorator.h"

namespace Gui
{

KMMTrackSoundPanel::KMMTrackSoundPanel(KdenliveApp *app,
					KTimeLine *timeline,
					KdenliveDoc *document,
					DocTrackSound *docTrack,
					bool isCollapsed, 
					QWidget *parent,
					const char *name ) :
						KMMTrackPanel(timeline, document, new KTrackPlacer(document, timeline, docTrack), SOUNDTRACK, parent,name),
						m_trackHeader( this, "Sound Track")
{
	m_trackHeader.trackLabel->setText(i18n("Sound Track"));
	m_trackIsCollapsed = isCollapsed;
	connect (m_trackHeader.collapseButton, SIGNAL(clicked()), this, SLOT(resizeTrack()));
	
	addFunctionDecorator("move", "resize");
	addFunctionDecorator("move", "move");
	addFunctionDecorator("move", "selectnone");
	addFunctionDecorator("razor", "razor");
	addFunctionDecorator("spacer", "spacer");
	addFunctionDecorator("marker", "marker");
	addFunctionDecorator("roll", "roll");

	decorateTrack();
	
}

KMMTrackSoundPanel::~KMMTrackSoundPanel()
{
}

void KMMTrackSoundPanel::resizeTrack()
{
	m_trackIsCollapsed = (!m_trackIsCollapsed);

	clearViewDecorators();
	decorateTrack();
	emit collapseTrack(this, m_trackIsCollapsed);

}

void KMMTrackSoundPanel::decorateTrack()
{
	uint widgetHeight;

	if (m_trackIsCollapsed) widgetHeight = collapsedTrackSize;
	else widgetHeight = KdenliveSettings::audiotracksize();

	setMinimumHeight(widgetHeight);
	setMaximumHeight(widgetHeight);

	if (KdenliveSettings::audiothumbnails() && !m_trackIsCollapsed)
	addViewDecorator(new TrackViewAudioBackgroundDecorator(timeline(), document(), KdenliveSettings::selectedaudioclipcolor(), KdenliveSettings::audioclipcolor()));

	else addViewDecorator(new TrackViewBackgroundDecorator(timeline(), document(), KdenliveSettings::selectedaudioclipcolor(), KdenliveSettings::audioclipcolor()));

	addViewDecorator(new TrackViewNameDecorator(timeline(), document()));

	if (m_trackIsCollapsed) m_trackHeader.collapseButton->setPixmap(KGlobal::iconLoader()->loadIcon("1downarrow",KIcon::Small,16));
	else m_trackHeader.collapseButton->setPixmap(KGlobal::iconLoader()->loadIcon("1rightarrow",KIcon::Small,16));
}

/*
void KMMTrackSoundPanel::paintClip(QPainter & painter, DocClipRef * clip, QRect &rect, bool selected)
{
	int sx = (int)timeLine()->mapValueToLocal(clip->trackStart().frames(25));
	int ex = (int)timeLine()->mapValueToLocal(clip->trackStart().frames(25) + clip->cropDuration().frames(25));

	if(sx < rect.x()) {
		sx = rect.x();
	}
	if(ex > rect.x() + rect.width()) {
		ex = rect.x() + rect.width();
	}
	ex -= sx;

	QColor col = selected ? QColor(64, 128, 64) : QColor(128, 255, 128);

	painter.fillRect( sx, rect.y(),
										ex, rect.height(),
										col);
	painter.drawRect( sx, rect.y(),
										ex, rect.height());
}
*/

} // namespace Gui
