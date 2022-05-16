/*
    SPDX-FileCopyrightText: 2008 Jean-Baptiste Mardelle <jb@kdenlive.org>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "../src/lib/localeHandling.h"
#include "mlt++/Mlt.h"
#include "renderjob.h"
#include <QApplication>
#include <QDir>
#include <QDomDocument>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStringList args = app.arguments();
    QStringList preargs;
    if (args.count() >= 4) {
        // Remove program name
        args.removeFirst();
        // renderer path (melt)
        QString render = args.at(0);
        args.removeFirst();
        // Source playlist path
        QString playlist = args.at(0);
        args.removeFirst();
        // target - where to save result
        QString target = args.at(0);
        args.removeFirst();
        int pid = 0;
        // pid to send back progress
        if (args.count() > 0 && args.at(0).startsWith(QLatin1String("-pid:"))) {
            pid = args.at(0).section(QLatin1Char(':'), 1).toInt();
            args.removeFirst();
        }
        int in = -1;
        int out = -1;
        
        if (args.count() > 1 && args.at(0) == QLatin1String("-out")) {
            args.removeFirst();
            out = args.at(0).toInt();
            args.removeFirst();
        }

        // Do we want a split render
        if (args.count() > 5 && args.at(0) == QLatin1String("-split")) {
            args.removeFirst();
            // chunks to render
            QStringList chunks = args.at(0).split(QLatin1Char(','), Qt::SkipEmptyParts);
            args.removeFirst();
            // chunk size in frames
            int chunkSize = args.at(0).toInt();
            args.removeFirst();
            // chunk size in frames
            QString profilePath = args.at(0);
            args.removeFirst();
            // rendered file extension
            QString extension = args.at(0);
            args.removeFirst();
            // avformat consumer params
            QStringList consumerParams = args.at(0).split(QLatin1Char(' '), Qt::SkipEmptyParts);
            args.removeFirst();
            QDir baseFolder(target);

            // After initialising the MLT factory, set the locale back from user default to C
            // to ensure numbers are always serialised with . as decimal point.
            Mlt::Factory::init();
            LocaleHandling::resetAllLocale();

            Mlt::Profile profile(profilePath.toUtf8().constData());
            profile.set_explicit(1);
            Mlt::Producer prod(profile, nullptr, playlist.toUtf8().constData());
            if (!prod.is_valid()) {
                fprintf(stderr, "INVALID playlist: %s \n", playlist.toUtf8().constData());
                return 1;
            }
            const char *localename = prod.get_lcnumeric();
            QLocale::setDefault(QLocale(localename));

            int currentFrame = 0;
            int rangeStart = 0;
            int rangeEnd = 0;
            QString frame;
            while (!chunks.isEmpty()) {
                if (rangeEnd == 0) {
                    // We are not processing a range
                    frame = chunks.first();
                }
                if (rangeEnd > 0) {
                    // We are processing a range
                    currentFrame += chunkSize + 1;
                    frame = QString::number(currentFrame);
                    if (currentFrame >= rangeEnd) {
                        // End of range
                        rangeStart = 0;
                        rangeEnd = 0;
                        // Range is processed, remove from stack
                        chunks.removeFirst();
                    }
                } else if (frame.contains(QLatin1Char('-'))) {
                    rangeStart = frame.section(QLatin1Char('-'), 0, 0).toInt();
                    rangeEnd = frame.section(QLatin1Char('-'), 1, 1).toInt();
                    currentFrame = rangeStart;
                    frame = QString::number(currentFrame);
                } else {
                    // Frame will be processed, remove from stack
                    chunks.removeFirst();
                }
                fprintf(stderr, "START:%d \n", frame.toInt());
                QString fileName = QStringLiteral("%1.%2").arg(frame,extension);
                if (baseFolder.exists(fileName)) {
                    // Don't overwrite an existing file
                    fprintf(stderr, "DONE:%d \n", frame.toInt());
                    continue;
                }
                QScopedPointer<Mlt::Producer> playlst(prod.cut(frame.toInt(), frame.toInt() + chunkSize));
                QScopedPointer<Mlt::Consumer> cons(
                    new Mlt::Consumer(profile, QString("avformat:%1").arg(baseFolder.absoluteFilePath(fileName)).toUtf8().constData()));
                for (const QString &param : qAsConst(consumerParams)) {
                    if (param.contains(QLatin1Char('='))) {
                        cons->set(param.section(QLatin1Char('='), 0, 0).toUtf8().constData(), param.section(QLatin1Char('='), 1).toUtf8().constData());
                    }
                }
                if (!cons->is_valid()) {
                    fprintf(stderr, " = =  = INVALID CONSUMER\n\n");
                    return 1;
                }
                cons->set("terminate_on_pause", 1);
                cons->connect(*playlst);
                playlst.reset();
                cons->run();
                cons->stop();
                cons->purge();
                fprintf(stderr, "DONE:%d \n", frame.toInt());
            }
            // Mlt::Factory::close();
            fprintf(stderr, "+ + + RENDERING FINISHED + + + \n");
            return 0;
        }
        
        // older MLT version, does not support embedded consumer in/out in xml, and current 
        // MLT (6.16) does not pass it onto the multi / movit consumer, so read it manually and enforce
        LocaleHandling::resetAllLocale();
        QFile f(playlist);
        QDomDocument doc;
        doc.setContent(&f, false);
        f.close();
        QDomElement consumer = doc.documentElement().firstChildElement(QStringLiteral("consumer"));
        if (!consumer.isNull()) {
            if (consumer.hasAttribute(QLatin1String("s")) || consumer.hasAttribute(QLatin1String("r"))) {
                // Workaround MLT embedded consumer resize (MLT issue #453)
                playlist.prepend(QStringLiteral("xml:"));
                playlist.append(QStringLiteral("?multi=1"));
            }
        }
        auto *rJob = new RenderJob(render, playlist, target, pid, in, out, qApp);
        rJob->start();
        QObject::connect(rJob, &RenderJob::renderingFinished, rJob, [&]() {
            rJob->deleteLater();
            app.quit();
        });
        return app.exec();
    } else {
        fprintf(stderr,
                "Kdenlive video renderer for MLT.\nUsage: "
                "kdenlive_render [-erase] [-kuiserver] [-locale:LOCALE] [in=pos] [out=pos] [render] [profile] [rendermodule] [player] [src] [dest] [[arg1] "
                "[arg2] ...]\n"
                "  -erase: if that parameter is present, src file will be erased at the end\n"
                "  -kuiserver: if that parameter is present, use KDE job tracker\n"
                "  -locale:LOCALE : set a locale for rendering. For example, -locale:fr_FR.UTF-8 will use a french locale (comma as numeric separator)\n"
                "  in=pos: start rendering at frame pos\n"
                "  out=pos: end rendering at frame pos\n"
                "  render: path to MLT melt renderer\n"
                "  profile: the MLT video profile\n"
                "  rendermodule: the MLT consumer used for rendering, usually it is avformat\n"
                "  player: path to video player to play when rendering is over, use '-' to disable playing\n"
                "  src: source file (usually MLT XML)\n"
                "  dest: destination file\n"
                "  args: space separated libavformat arguments\n");
        return 1;
    }
}
