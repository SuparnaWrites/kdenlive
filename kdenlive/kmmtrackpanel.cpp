/***************************************************************************
                          kmmtrackpanel.cpp  -  description
                             -------------------
    begin                : Tue Aug 6 2002
    copyright            : (C) 2002 by Jason Wood
    email                : jasonwood@blueyonder.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kmmtrackpanel.h"
#include "kmmtimeline.h"

KMMTrackPanel::KMMTrackPanel(KMMTimeLine &timeline, DocTrackBase & docTrack, QWidget *parent, const char *name) :
					        			QFrame(parent,name),
												m_docTrack(docTrack),
												m_timeline(timeline)
{
		setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum));
  	setPalette( QPalette( QColor(170, 170, 170) ) );

		setFrameStyle(QFrame::Panel | QFrame::Sunken);
}

KMMTrackPanel::~KMMTrackPanel()
{
}

/** Read property of KMMTimeLine * m_timeline. */
KMMTimeLine & KMMTrackPanel::timeLine()
{
	return m_timeline;
}

/** returns the document track which is displayed by this track */
DocTrackBase & KMMTrackPanel::docTrack()
{
	return m_docTrack;
}

/** Draws the trackto a back buffered QImage, but does not display it. This image can then be
blitted straight to the screen for speedy drawing. */
void KMMTrackPanel::drawToBackBuffer(QPainter &painter, QRect &rect)
{
	double startValue = timeLine().mapLocalToValue(0.0);
	double endValue = timeLine().mapLocalToValue(rect.width());

	QPtrListIterator<DocClipBase> clip = docTrack().firstClip(startValue, endValue);

	DocClipBase *endClip = docTrack().endClip(startValue, endValue).current();

	for(DocClipBase *curClip; (curClip = clip.current())!=endClip; ++clip) {
		paintClip(painter, curClip, rect, m_timeline.clipSelected(curClip));
	}
}
