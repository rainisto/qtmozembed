/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QGraphicsMozView"

#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include <QTimerEvent>
#include <QThread>
#include <QtOpenGL/QGLContext>

#include "qgraphicsmozview.h"
#include "qmozcontext.h"
#include "InputData.h"
#include "qmozembedlog.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

#include "qgraphicsmozview_p.h"

using namespace mozilla;
using namespace mozilla::embedlite;

QGraphicsMozView::QGraphicsMozView(QGraphicsItem* parent)
    : QGraphicsWidget(parent)
    , d(new QGraphicsMozViewPrivate(new IMozQView<QGraphicsMozView>(*this), this))
    , mParentID(0)
    , mUseQmlMouse(false)
{
    setObjectName("QGraphicsMozView");
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    setAcceptDrops(true);
    setAcceptTouchEvents(true);
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setFlag(QGraphicsItem::ItemIsFocusScope, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setInputMethodHints(Qt::ImhPreferLowercase);

    d->mContext = QMozContext::GetInstance();
    connect(this, SIGNAL(updateThreaded()), this, SLOT(OnUpdateThreaded()));
    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(onInitialized()));
    } else {
        QTimer::singleShot(0, this, SLOT(onInitialized()));
    }
}

void
QGraphicsMozView::setParentID(unsigned aParentID)
{
    LOGT("mParentID:%u", aParentID);
    mParentID = aParentID;
    if (mParentID) {
        onInitialized();
    }
}

QGraphicsMozView::~QGraphicsMozView()
{
    if (d->mView) {
        d->mView->SetListener(NULL);
        d->mContext->GetApp()->DestroyView(d->mView);
    }
    delete d;
}

void
QGraphicsMozView::onInitialized()
{
    LOGT("mParentID:%u", mParentID);
    if (!d->mView) {
        d->mView = d->mContext->GetApp()->CreateView(mParentID);
        d->mView->SetListener(d);
    }
}

quint32
QGraphicsMozView::uniqueID() const
{
    return d->mView ? d->mView->GetUniqueID() : 0;
}

void
QGraphicsMozView::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget*)
{
    return;
}

void
QGraphicsMozView::CompositingFinished()
{
}

bool
QGraphicsMozView::Invalidate()
{
    return false;
}

void
QGraphicsMozView::OnUpdateThreaded()
{
    update();
}

void
QGraphicsMozView::createGeckoGLContext()
{
    Q_EMIT requestGLContextQGV(d->mHasContext, d->mGLSurfaceSize);
}

void
QGraphicsMozView::requestGLContext(bool& hasContext, QSize& viewPortSize)
{
    Q_EMIT requestGLContextQGV(d->mHasContext, d->mGLSurfaceSize);
    hasContext = d->mHasContext;
    viewPortSize = d->mGLSurfaceSize;
}

void QGraphicsMozView::drawUnderlay()
{
    // Do nothing
}

void QGraphicsMozView::drawOverlay(const QRect &rect) {
    Q_UNUSED(rect);
    // Do nothing;
}

/*! \reimp
*/
QSizeF QGraphicsMozView::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
{
    if (which == Qt::PreferredSize)
        return QSizeF(800, 600); // ###
    return QGraphicsWidget::sizeHint(which, constraint);
}

/*! \reimp
*/
void QGraphicsMozView::setGeometry(const QRectF& rect)
{
    QGraphicsWidget::setGeometry(rect);

    // NOTE: call geometry() as setGeometry ensures that
    // the geometry is within legal bounds (minimumSize, maximumSize)
    d->mSize = geometry().size();
    Q_EMIT requestGLContext(d->mHasContext, d->mGLSurfaceSize);
    d->UpdateViewSize();
}

QUrl QGraphicsMozView::url() const
{
    return QUrl(d->mLocation);
}

void QGraphicsMozView::setUrl(const QUrl& url)
{
    load(url.toString());
}

void QGraphicsMozView::load(const QString& url)
{
    if (url.isEmpty())
        return;

    if (!d->mViewInitialized) {
        return;
    }
    LOGT("url: %s", url.toUtf8().data());
    d->mProgress = 0;
    d->mView->LoadURL(url.toUtf8().data());
}

void QGraphicsMozView::loadFrameScript(const QString& name)
{
    LOGT("script:%s", name.toUtf8().data());
    d->mView->LoadFrameScript(name.toUtf8().data());
}

void QGraphicsMozView::addMessageListener(const QString& name)
{
    if (!d->mViewInitialized)
        return;

    LOGT("name:%s", name.toUtf8().data());
    d->mView->AddMessageListener(name.toUtf8().data());
}

void QGraphicsMozView::addMessageListeners(const QStringList& messageNamesList)
{
    if (!d->mViewInitialized)
        return;

    nsTArray<nsString> messages;
    for (int i = 0; i < messageNamesList.size(); i++) {
        messages.AppendElement((char16_t*)messageNamesList.at(i).data());
    }
    d->mView->AddMessageListeners(messages);
}

void QGraphicsMozView::sendAsyncMessage(const QString& name, const QVariant& variant)
{
    d->sendAsyncMessage(name, variant);
}

QPointF QGraphicsMozView::scrollableOffset() const
{
    return d->mScrollableOffset;
}

bool QGraphicsMozView::isPainted() const
{
    return d->mIsPainted;
}

float QGraphicsMozView::resolution() const
{
    return d->mContentResolution;
}

QRectF QGraphicsMozView::contentRect() const
{
    return d->mContentRect;
}

QSizeF QGraphicsMozView::scrollableSize() const
{
    return d->mScrollableSize;
}

QString QGraphicsMozView::title() const
{
    return d->mTitle;
}

int QGraphicsMozView::loadProgress() const
{
    return d->mProgress;
}

bool QGraphicsMozView::canGoBack() const
{
    return d->mCanGoBack;
}

bool QGraphicsMozView::canGoForward() const
{
    return d->mCanGoForward;
}

bool QGraphicsMozView::loading() const
{
    return d->mIsLoading;
}

QColor QGraphicsMozView::bgcolor() const
{
    return d->mBgColor;
}

bool QGraphicsMozView::pinching() const{
    return d->mPinching;
}

void QGraphicsMozView::loadHtml(const QString& html, const QUrl& baseUrl)
{
    LOGT();
}

void QGraphicsMozView::goBack()
{
    LOGT();
    if (!d->mViewInitialized)
        return;
    d->mView->GoBack();
}

void QGraphicsMozView::goForward()
{
    LOGT();
    if (!d->mViewInitialized)
        return;
    d->mView->GoForward();
}

void QGraphicsMozView::stop()
{
    LOGT();
    if (!d->mViewInitialized)
        return;
    d->mView->StopLoad();
}

void QGraphicsMozView::reload()
{
    LOGT();
    if (!d->mViewInitialized)
        return;
    d->mView->Reload(false);
}

bool QGraphicsMozView::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        QTouchEvent* touch = static_cast<QTouchEvent*>(event);
        if (!mUseQmlMouse || touch->touchPoints().size() > 1) {
            d->touchEvent(touch);
        }
        return true;
    }
    case QEvent::Show: {
        LOGT("Event Show: curCtx:%p", (void*)QGLContext::currentContext());
        break;
    }
    case QEvent::Hide: {
        LOGT("Event Hide");
        break;
    }
    default:
        break;
    }

    // Here so that it can be reimplemented without breaking ABI.
    return QGraphicsWidget::event(event);
}

void QGraphicsMozView::suspendView()
{
    if (!d->mViewInitialized) {
        return;
    }
    d->mView->SetIsActive(false);
    d->mView->SuspendTimeouts();
}

void QGraphicsMozView::resumeView()
{
    if (!d->mViewInitialized) {
        return;
    }
    d->mView->SetIsActive(true);
    d->mView->ResumeTimeouts();
}

bool QGraphicsMozView::getUseQmlMouse()
{
    return mUseQmlMouse;
}

void QGraphicsMozView::setUseQmlMouse(bool value)
{
    mUseQmlMouse = value;
}

void QGraphicsMozView::recvMouseMove(int posX, int posY)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_MOVE, QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
        event.mTouches.AppendElement(SingleTouchData(0,
                                     mozilla::ScreenIntPoint(posX, posY),
                                     mozilla::ScreenSize(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
    }
}

void QGraphicsMozView::recvMousePress(int posX, int posY)
{
    forceViewActiveFocus();
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_START, QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
        event.mTouches.AppendElement(SingleTouchData(0,
                                     mozilla::ScreenIntPoint(posX, posY),
                                     mozilla::ScreenSize(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
    }
}

void QGraphicsMozView::recvMouseRelease(int posX, int posY)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_END, QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
        event.mTouches.AppendElement(SingleTouchData(0,
                                     mozilla::ScreenIntPoint(posX, posY),
                                     mozilla::ScreenSize(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
    }
    if (d->mPendingTouchEvent) {
        d->mPendingTouchEvent = false;
    }
}

void QGraphicsMozView::forceViewActiveFocus()
{
    QGraphicsItem *parent = parentItem();
    while (parent) {
        if (parent->flags() & QGraphicsItem::ItemIsFocusScope)
            parent->setFocus(Qt::OtherFocusReason);
        parent = parent->parentItem();
    }

    setFocus(Qt::OtherFocusReason);
    if (d->mViewInitialized) {
        d->mView->SetIsActive(true);
    }
}

void QGraphicsMozView::focusInEvent(QFocusEvent* event)
{
    d->SetIsFocused(true);
    QGraphicsWidget::focusInEvent(event);
}

void QGraphicsMozView::focusOutEvent(QFocusEvent* event)
{
    d->SetIsFocused(false);
    QGraphicsWidget::focusOutEvent(event);
}

void QGraphicsMozView::timerEvent(QTimerEvent *event)
{
    d->timerEvent(event);
    if (!event->isAccepted()) {
        QGraphicsView::timerEvent(event);
    }
}


void QGraphicsMozView::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (!mUseQmlMouse) {
        const bool accepted = e->isAccepted();
        recvMouseMove(e->pos().x(), e->pos().y());
        e->setAccepted(accepted);
    }
    else {
        QGraphicsWidget::mouseMoveEvent(e);
    }

    if (d->mViewInitialized && !d->mDragging && d->mPressed) {
        d->mDragging = true;
        d->mViewIface->draggingChanged();
    }
}

void QGraphicsMozView::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if (!mUseQmlMouse) {
        const bool accepted = e->isAccepted();
        recvMousePress(e->pos().x(), e->pos().y());
        e->setAccepted(accepted);
    }
    else {
        QGraphicsWidget::mousePressEvent(e);
    }

    d->mPressed = true;
}

void QGraphicsMozView::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    if (!mUseQmlMouse) {
        const bool accepted = e->isAccepted();
        recvMouseRelease(e->pos().x(), e->pos().y());
        e->setAccepted(accepted);
    }
    else {
        QGraphicsWidget::mouseReleaseEvent(e);
    }

    d->mPressed = false;
    if (d->mViewInitialized && d->mDragging) {
        d->mDragging = false;
        d->mViewIface->draggingChanged();
    }
}

void QGraphicsMozView::inputMethodEvent(QInputMethodEvent* event)
{
    d->inputMethodEvent(event);
}

void QGraphicsMozView::keyPressEvent(QKeyEvent* event)
{
    d->keyPressEvent(event);
}

void QGraphicsMozView::keyReleaseEvent(QKeyEvent* event)
{
    d->keyReleaseEvent(event);
}

QVariant
QGraphicsMozView::inputMethodQuery(Qt::InputMethodQuery aQuery) const
{
    static bool commitNow = getenv("DO_FAST_COMMIT") != 0;
    return commitNow ? QVariant(0) : QVariant();
}

void
QGraphicsMozView::newWindow(const QString& url)
{
    LOGT("New Window: %s", url.toUtf8().data());
}

void
QGraphicsMozView::synthTouchBegin(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START, QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        mozilla::ScreenIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           mozilla::ScreenSize(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}

void
QGraphicsMozView::synthTouchMove(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_MOVE, QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        mozilla::ScreenIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           mozilla::ScreenSize(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}

void
QGraphicsMozView::synthTouchEnd(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_END, QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        mozilla::ScreenIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           mozilla::ScreenSize(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}
