#pragma once

#include <QThread>
#include <QImage>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include "../hooks/idisassemblercommand.h"

class RendererAsync: public QThread
{
    Q_OBJECT

    private:
        typedef std::unique_lock<std::mutex> renderer_lock;

    public:
        RendererAsync(const RDDisassemblerPtr& disassembler, QObject* parent);
        virtual ~RendererAsync();
        void abort();

    signals:
        void renderCompleted(const QImage& image);

    protected:
        QWidget* widget() const;
        virtual bool conditionWait() const;
        virtual void onRender(QImage* image) = 0;
        void schedule(QThread::Priority priority = InheritPriority);
        void run() override;

    protected:
        RDDisassemblerPtr m_disassembler;

    private:
        std::atomic_bool m_abort{false}, m_painting{false};
        std::condition_variable m_cv;
        std::mutex m_mutex;
};

